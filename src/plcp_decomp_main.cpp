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
            typedef IntegerFileArray<uint40_t> text_t;

            const size_t nFactors = text.size() / 2;
            uint40_t nReferences = 0;
            uint40_t nLiterals = 0;

            // count literals and number of characters contained in references
            for (size_t i = 0; i < nFactors; i++)
            {
                uint40_t len = swapBytes(text[2 * i + 1]);
                if(len == 0) nLiterals += 1;
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
                if(len == 0) {
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

    tdc::lcpcomp::defactorize(infile, outfile, mb_ram);

    return 0;
}

