#include <algorithm>
#include <iostream>
#include <fstream>
#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/sort.h>

namespace tdc { namespace lcpcomp {
    constexpr len_t M = 1024 * 1024;
    using uint40_t = uint_t<40>;
    using uint40pair_t = std::pair<uint40_t,uint40_t>;
    using uint40triple_t = std::tuple<uint40_t,uint40_t,uint40_t>;

    inline uint40_t textPos(const uint40triple_t &factor) {
        return std::get<0>(factor);
    }
    
    inline uint40_t targetPos(const uint40triple_t &factor) {
        return std::get<1>(factor);
    }

    inline uint40_t factorLength(const uint40triple_t &factor) {
        return std::get<2>(factor);
    }
    
    inline void textPos(uint40triple_t &factor, uint40_t value) {
        std::get<0>(factor) = value;
    }
    
    inline void targetPos(uint40triple_t &factor, uint40_t value) {
        std::get<1>(factor) = value;
    }

    inline void factorLength(uint40triple_t &factor, uint40_t value) {
        std::get<2>(factor) = value;
    }
    
    void print(const uint40triple_t &triple, const std::string &header) {
        std::cout << header << " { " << textPos(triple) << ", " << targetPos(triple) << ", " << factorLength(triple) << " }" << std::endl;
    }


    inline uint40_t swapBytes(uint40_t value) {
        uint40_t result = 0;
        result |= (value & 0xFF) << 32;
        result |= (value & 0xFF00) << 16;
        result |= (value & 0xFF0000);
        result |= (value & 0xFF000000) >> 16;
        result |= (value & (uint40_t)0xFF00000000) >> 32;
        return result;
    }
    
    template <bool sortByTarget = false>
    struct SingleCharacterFactorComparison
    {
        const uint40_t min = std::numeric_limits<uint40_t>::min();
        const uint40_t max = std::numeric_limits<uint40_t>::max();
        
        const uint40pair_t min_factor = std::make_pair(min, min);
        const uint40pair_t max_factor = std::make_pair(max, max);

        bool operator()(const uint40pair_t &a, const uint40pair_t &b) const {
            return std::get<sortByTarget>(a) < std::get<sortByTarget>(b);
        }
        uint40pair_t min_value() const
        { return min_factor; }
        uint40pair_t max_value() const
        { return max_factor; }
    };
    
    template <bool sortByTarget = false>
    struct CopyFactorComparison
    {
        const uint40_t min = std::numeric_limits<uint40_t>::min();
        const uint40_t max = std::numeric_limits<uint40_t>::max();
        
        const uint40triple_t min_factor = std::make_tuple(min, min, min);
        const uint40triple_t max_factor = std::make_tuple(max, max, max);

        bool operator()(const uint40triple_t &a, const uint40triple_t &b) const {
            return std::get<sortByTarget>(a) < std::get<sortByTarget>(b);
        }
        uint40triple_t min_value() const
        { return min_factor; }
        uint40triple_t max_value() const
        { return max_factor; }
    };
    
    inline void defactorize(const std::string& textfilename,
                          const std::string& outfilename,
                          const len_t mb_ram) {      

        StatPhase phase("PLCPDeComp");
        StatPhase::wrap("Decompress", [&]{
            
            SingleCharacterFactorComparison<false> sortByTextPos;
            SingleCharacterFactorComparison<true> sortByTargetPos;      

            // (textPos, character) pairs
            stxxl::VECTOR_GENERATOR<uint40pair_t>::result m_literals;
            
            
            // (textPos, targetPos) pairs
            // this vector will always be sorted by textPos
            stxxl::VECTOR_GENERATOR<uint40pair_t>::result m_references1;
            // (textPos, targetPos) pairs
            // this vector will always be sorted by targetPos
            stxxl::VECTOR_GENERATOR<uint40pair_t>::result m_references2;


            IntegerFileArray<uint40_t> text (textfilename.c_str());

            const size_t nFactors = text.size() / 2;
            uint40_t nReferences = 0;
            uint40_t nLiterals = 0;

            // count literals and number of characters contained in references
            for (size_t i = 0; i < nFactors; i++)
            {
                uint40_t len = swapBytes(text[2 * i + 1]);
                if(len == uint40_t(0)) nLiterals += 1;
                else nReferences += len;
            }

            // reserve space on disk for vectors
            m_literals.reserve(nLiterals + nReferences);
            m_references1.reserve(nReferences);
            m_references2.reserve(nReferences);

            // everything here is 1-based
            // (necessary for stxxl sort)
            uint40_t textPos = 1;
            for (size_t i = 0; i < nFactors; i++)
            {
                // read current factor
                uint40_t target = swapBytes(text[i * 2]) + 1;
                uint40_t len = swapBytes(text[i * 2 + 1]);
                if(len == uint40_t(0)) {
                    // fill literal vector (naturally sorted by text position)
                    m_literals.push_back(std::make_pair(textPos++, target));
                } 
                else {
                    // fill references vector (naturally sorted by text position)
                    for (uint40_t j = 0; j < len; j++)
                    {
                        auto reference = std::make_pair(textPos++, target++);
                        m_references2.push_back(reference);
                    }
                }
            } 
            
            // do pointer jumping until there are no more changes
            // (this implementation is quite naive so far)
            bool changed = true;
            while(changed) {
                changed = false;
                // copy changes from updated reference to original references
                m_references1 = m_references2;
                // sort first references by textPos, second references by targetPos
                stxxl::sort(m_references1.begin(), m_references1.end(), sortByTextPos, mb_ram*1024*1024);
                stxxl::sort(m_references2.begin(), m_references2.end(), sortByTargetPos, mb_ram*1024*1024);

                // do the actual pointer jumping
                uint40_t j = 0;            
                for (uint40_t i = 0; i < nReferences; i++)
                {
                    while(m_references1[j].first < m_references2[i].second) j++;
                    if(m_references1[j].first == m_references2[i].second) {
                        if(m_references2[i].second != m_references1[j].second) {
                            m_references2[i].second = m_references1[j].second;
                            changed = true;
                        }
                    }
                }
            }
            
            // replace the targetPos of the reference by actual literals
            // push everything into the literal vector
            uint40_t j = 0;
            for (uint40_t i = 0; i < nReferences; i++)
            {
                while(m_literals[j].first < m_references2[i].second) j++;
                auto literal = std::make_pair(m_references2[i].first, m_literals[j].second);
                m_literals.push_back(literal);
            }           
            // sort the literal vector by text position 
            stxxl::sort(m_literals.begin(), m_literals.end(), sortByTextPos, mb_ram*1024*1024);
            
            // write only the actual literal characters into the output file
            std::ofstream outputFile;
            outputFile.open(outfilename);
            for (auto literal : m_literals)
                outputFile << char(literal.second - 1);
            outputFile.close();
        });

    }
    
    
    inline void defactorize2(const std::string& textfilename,
                          const std::string& outfilename,
                          const len_t mb_ram) {      

        StatPhase phase("PLCPDeComp");
        StatPhase::wrap("Decompress", [&]{
            
            CopyFactorComparison<false> sortByTextPos;
            CopyFactorComparison<true> sortByTargetPos;      

            stxxl::VECTOR_GENERATOR<uint40pair_t>::result m_literals;
            stxxl::VECTOR_GENERATOR<uint40triple_t>::result byTextPosV;
            stxxl::VECTOR_GENERATOR<uint40triple_t>::result byTargetPosV;            
            stxxl::VECTOR_GENERATOR<uint40triple_t>::result byTargetPosV_new;
            stxxl::VECTOR_GENERATOR<uint40triple_t>::result resolvedV;
            stxxl::VECTOR_GENERATOR<uint40triple_t>::result resolvedV_new;

            IntegerFileArray<uint40_t> text (textfilename.c_str());

            const size_t nFactors = text.size() / 2;
            uint40_t nReferences = 0;
            uint40_t nLiterals = 0;

            // count literals and number of characters contained in references
            for (size_t i = 0; i < nFactors; i++) {
                uint40_t len = swapBytes(text[2 * i + 1]);
                if(len == uint40_t(0)) nLiterals += 1;
                else nReferences += len;
            }

            // reserve space on disk for vectors
            m_literals.reserve(nLiterals + nReferences);
            byTextPosV.reserve(nReferences);
            byTargetPosV.reserve(nReferences);
            byTargetPosV_new.reserve(nReferences);
            resolvedV.reserve(nReferences);
            resolvedV_new.reserve(nReferences);
            
            // everything here is 1-based
            // (necessary for stxxl sort)
            uint40_t textPosition = 1;
            for (size_t i = 0; i < nFactors; i++) {
                // read current factor
                uint40_t target = swapBytes(text[i * 2]) + 1;
                uint40_t len = swapBytes(text[i * 2 + 1]);
                if(len == uint40_t(0)) {
                    // fill literal vector (naturally sorted by text position)
                    m_literals.push_back(std::make_pair(textPosition++, target));
                } 
                else {
                    // fill requests vector (naturally sorted by text position)
                    auto reference = std::make_tuple(textPosition, target++, len);
                    byTargetPosV.push_back(reference);
                    textPosition += len;
                }
            } 
            
            
            // do pointer jumping until there are no more unresolveds factors
            while(byTargetPosV.size() > 0) {
                
                std::cout << "Factors left: " << byTargetPosV.size() << std::endl;
                
                byTextPosV = byTargetPosV;
                stxxl::sort(byTextPosV.begin(), byTextPosV.end(), sortByTextPos, mb_ram*1024*1024);
                stxxl::sort(byTargetPosV.begin(), byTargetPosV.end(), sortByTargetPos, mb_ram*1024*1024);
                
                unsigned j = 0;
                unsigned last_j = 0;
                uint40_t last_targetStart = 0;
                uint40triple_t &byTextPos = byTextPosV[j];
                for(unsigned i = 0; i < byTargetPosV.size(); i++) {         
                    
                    uint40triple_t &byTargetPos = byTargetPosV[i];
                    
                    print(byTargetPos, "   Factor " + std::to_string(i) + ":");
                    
                    uint40_t targetLen = factorLength(byTargetPos);
                    uint40_t targetStart = targetPos(byTargetPos);
                    uint40_t targetEnd = targetStart + targetLen;
                    
                    if(last_targetStart > targetStart) {
                        j = last_j;
                        byTextPos = byTextPosV[j];
                    }
                    
                    while(targetStart >= textPos(byTextPos) + factorLength(byTextPos))
                        byTextPos = byTextPosV[++j];
                        
                    print(byTextPos, "       Against:");
                        
                    uint40_t textLen = factorLength(byTextPos);
                    uint40_t textStart = textPos(byTextPos);
                    uint40_t textEnd = textStart + textLen;
                    
                    
                    uint40_t prefixLen;                  
                    
                    // resolve prefix or entire factor
                    if(targetStart < textStart) {
                        prefixLen = targetLen - (std::max(targetEnd, textStart) - textStart);
                        
                        uint40triple_t resolved = std::make_tuple(textPos(byTargetPos), targetPos(byTargetPos), prefixLen);
                        resolvedV_new.push_back(resolved);
                        
                        print(resolved, "           Resolved:");
                    }
                    // pointerjump prefix or entire factor
                    else {                       
                        
                        auto posOffset = targetStart - textStart;
                        auto newTargetPos = targetPos(byTextPos) + posOffset;
                        prefixLen = targetLen - (std::max(targetEnd, textEnd) - textEnd);
                        
                        uint40triple_t jumped = std::make_tuple(textPos(byTargetPos), newTargetPos, prefixLen);
                        byTargetPosV_new.push_back(jumped);
                        
                        print(jumped, "           Jumped:");
                    }
                    
                    if(prefixLen < targetLen) {
                        textPos(byTargetPos, textPos(byTargetPos) + prefixLen);
                        targetPos(byTargetPos, targetPos(byTargetPos) + prefixLen);
                        factorLength(byTargetPos, factorLength(byTargetPos) - prefixLen);
                        --i;
                        last_j = j;
                        print(byTargetPosV[i + 1], "           Remaining:");
                    }
                    
                    last_targetStart = targetStart;
                }
                
                for(auto resolved : resolvedV_new) {
                    resolvedV.push_back(resolved);
                }
                byTargetPosV = byTargetPosV_new;
                
                byTargetPosV_new.clear();
                resolvedV_new.clear();
            }
            
            for(auto resolved : resolvedV) {
                print(resolved, "Resolved:");
            }
        });

    }
}}//ns


bool file_exist(const std::string& filepath) {
    std::ifstream file(filepath);
    return !file.fail();
}


int main(int argc, char** argv) {
    if(argc <= 2) {
        std::cerr << "Usage : " << argv[0]
                  << " <infile> <outfile> [mb_ram]" << std::endl;
        return 1;
    }
    tdc::StatPhase root("Root");

    const std::string infile { argv[1] };
    const std::string outfile { argv[2] };
    if(!file_exist(infile)) {
        std::cerr << "Infile " << infile << " does not exist" << std::endl;
        return 1;
    }

    const tdc::len_t mb_ram = (argc >= 3) ? std::stoi(argv[3]) : 512;

    tdc::lcpcomp::defactorize2(infile, outfile, mb_ram);

    return 0;
}

