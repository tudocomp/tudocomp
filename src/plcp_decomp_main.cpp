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
#include <stxxl/bits/io/iostats.h>

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
            
            // generate stats instance
            stxxl::stats * Stats = stxxl::stats::get_instance();
            // start measurement here
            stxxl::stats_data stats_begin(*Stats);            
            
            SingleCharacterFactorComparison<false> sortByTextPos;
            SingleCharacterFactorComparison<true> sortByTargetPos;
            
            // vector to hold the restored text
            stxxl::VECTOR_GENERATOR<char>::result restoredText;
            
            // (textPos, targetPos) pairs:
            
            typedef stxxl::VECTOR_GENERATOR<uint40pair_t>::result references_vector;

            // this vector will always be sorted by textPos
            references_vector byTextPos;
            // this vector will always be sorted by targetPos
            references_vector *byTargetPos = new references_vector();
            // this vector will always be sorted by targetPos
            references_vector *byTargetPosNew = new references_vector();


            IntegerFileArray<uint40_t> text (textfilename.c_str());

            const size_t nFactors = text.size() / 2;
            uint40_t nReferences = 0;
            uint40_t nLiterals = 0;
            uint40_t nCharacters;

            // count literals and number of characters contained in references
            for (size_t i = 0; i < nFactors; i++)
            {
                uint40_t len = swapBytes(text[2 * i + 1]);
                if(len == uint40_t(0)) nLiterals += 1;
                else nReferences += len;
            }
            nCharacters = nReferences + nLiterals;
            
            std::cout << "Factors:              " << nFactors << std::endl;
            std::cout << "Literal characters:   " << nLiterals << std::endl;
            std::cout << "Reference characters: " << nReferences << std::endl;
            std::cout << "Total characters:     " << nCharacters << std::endl;

            // reserve space on disk for vectors
            restoredText.resize(nCharacters);
            byTargetPos->reserve(nReferences);

            std::cout << "Reserved space for vectors." << std::endl;

            // everything here is 1-based
            // (necessary for stxxl sort)
            uint40_t textPos = 1;
            for (size_t i = 0; i < nFactors; i++)
            {
                // read current factor
                uint40_t target = swapBytes(text[i * 2]) + 1;
                uint40_t len = swapBytes(text[i * 2 + 1]);
                if(len == uint40_t(0)) {
                    restoredText[textPos++ - 1] = char(target - 1);
                } 
                else {
                    // fill references vector (naturally sorted by text position)
                    for (uint40_t j = 0; j < len; j++)
                    {
                        auto reference = std::make_pair(textPos++, target++);
                        byTargetPos->push_back(reference);
                    }
                }
            } 
            
            std::cout << "Vectors filled: " << byTargetPos->size() << std::endl;
            
            int round = 0;
            // do pointer jumping until there are no more changes
            // (this implementation is quite naive so far)
            while(byTargetPos->size() > 0) {
                round++;
                std::cout << "Round " << round << ": Remaining references: " << byTargetPos->size() << std::endl;
                
                std::cout << "Copying modified byTargetPos vector to byTextPos vector..." << std::endl;
                byTextPos = *byTargetPos;
                
                std::cout << "Sorting byTargetPos vector..." << std::endl;
                stxxl::sort(byTargetPos->begin(), byTargetPos->end(), sortByTargetPos, mb_ram*1024*1024);
                std::cout << "Sorting byTextPos vector..." << std::endl;
                if(round > 1) stxxl::sort(byTextPos.begin(), byTextPos.end(), sortByTextPos, mb_ram*1024*1024);
                
                // add guard to byTextPos vector
                byTextPos.push_back(sortByTextPos.max_value());
                
                std::cout << "Initializing buffer for pointer-jumped references..." << std::endl;
                byTargetPosNew->clear();
                byTargetPosNew->reserve(byTargetPos->size());

                std::cout << "Scanning (Resolve & Jump)..." << std::endl;
                uint40_t jumped = 0, resolved = 0;
                // do the actual pointer jumping
                
                references_vector::bufreader_type i_iter(byTextPos);            
                for (references_vector::bufreader_type j_iter(*byTargetPos); !j_iter.empty(); ++j_iter)
                {
                    while((*i_iter).first < (*j_iter).second) ++i_iter;

                    //~ std::cout << "Round " << round << ": Entry i: " << (*i_iter).first << ", Entry j: " << (*j_iter).second << ", Changes: " << changes << " ---" << std::endl;

                    auto referenceFromByTextPos = (*i_iter);
                    auto referenceFromByTargetPos = (*j_iter);
                    
                    if(referenceFromByTextPos.first == referenceFromByTargetPos.second) {
                        // jump
                        referenceFromByTargetPos.second = referenceFromByTextPos.second;
                        byTargetPosNew->push_back(referenceFromByTargetPos);
                        jumped++;
                    } else {
                        // restore
                        restoredText[referenceFromByTargetPos.first - 1] = 
                            restoredText[referenceFromByTargetPos.second - 1];
                        resolved++;
                    }                    
                }
                
                
                std::cout << "Pointing byTargetPos to buffer..." << std::endl;
                
                stxxl::VECTOR_GENERATOR<uint40pair_t>::result *h = byTargetPos;
                byTargetPos = byTargetPosNew;
                byTargetPosNew = h;
                
                std::cout << "Round complete. Resolved: " << resolved << ". Jumped: " << jumped << "." << std::endl;
                
                // substract current stats from stats at the beginning of the measurement
                std::cout << (stxxl::stats_data(*Stats) - stats_begin) << std::endl;
                
            }
            delete byTargetPosNew;
            
            
            
            // write restored text into output file
            std::ofstream outputFile;
            outputFile.open(outfilename);
            for (auto character : restoredText)
                outputFile << character;
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

            // vector to hold the restored text
            stxxl::VECTOR_GENERATOR<char>::result restoredText;
            // vector for all unresolved factors sorted by target position
            stxxl::VECTOR_GENERATOR<uint40triple_t>::result byTargetPosV;
            // vector for all unresolved factors sorted by text position
            stxxl::VECTOR_GENERATOR<uint40triple_t>::result byTextPosV;
            // vector for the factors that have been resolved during the current scan
            stxxl::VECTOR_GENERATOR<uint40triple_t>::result resolvedV;

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

            std::cout << "Factors:              " << nFactors << std::endl;
            std::cout << "Literal characters:   " << nLiterals << std::endl;
            std::cout << "Reference characters: " << nReferences << std::endl;

            // reserve space on disk for vectors
            restoredText.resize(nLiterals + nReferences);
            byTargetPosV.reserve(nReferences);
            byTextPosV.reserve(nReferences);
            resolvedV.reserve(nReferences);
            
            // everything here is 1-based
            // (necessary for stxxl sort)
            uint40_t textPosition = 1;
            for (size_t i = 0; i < nFactors; i++) {
                // read current factor
                uint40_t target = swapBytes(text[i * 2]) + 1;
                uint40_t len = swapBytes(text[i * 2 + 1]);
                if(len == uint40_t(0)) {
                    // restore literal factors immediatly
                    // (naturally in textpos order -> single scan)
                    restoredText[textPosition++ - 1] = char(target - 1);
                } 
                else {
                    // fill one of the factor vectors
                    // (other one will get copied)
                    auto reference = std::make_tuple(textPosition, target, len);
                    byTargetPosV.push_back(reference);
                    textPosition += len;
                }
            } 
            
            auto byTargetPosSize = byTargetPosV.size();
            long byTargetPosResize = 0;
            
            int round = 0;
            // do pointer jumping and resolving until there are no more unresolved factors
            while(byTargetPosSize + byTargetPosResize > 0) {
                
                // sort unresolved vectors by target pos
                stxxl::sort(byTargetPosV.begin(), byTargetPosV.end(), sortByTargetPos, mb_ram*1024*1024);
                
                // remove old factors that have been either entirely resolved or pointer-jumped
                byTargetPosV.resize(byTargetPosSize + byTargetPosResize);
                byTargetPosResize = 0;
                
                // copy unresolved factors and sort copy by textpos
                byTextPosV = byTargetPosV;
                stxxl::sort(byTextPosV.begin(), byTextPosV.end(), sortByTextPos, mb_ram*1024*1024);
                
                // memorize number of unresolved factors
                // (will be used for resizing in the following round)
                byTargetPosSize = byTargetPosV.size();
                
                std::cout << "Round " << round++ << ": Factors left: " << byTargetPosSize << std::endl;
                
                unsigned j = 0;
                unsigned last_j = 0;
                uint40_t last_targetStart = 0;
                uint40triple_t &byTextPos = byTextPosV[j];
                for(unsigned i = 0; i < byTargetPosSize; i++) {
                    
                    // try to resolve or pointer jump the factors in targetpos order
                    uint40triple_t &byTargetPos = byTargetPosV[i];
                    
                    uint40_t targetLen = factorLength(byTargetPos);
                    uint40_t targetStart = targetPos(byTargetPos);
                    uint40_t targetEnd = targetStart + targetLen;
                    
                    // rollback if splitting up factors has resulted in moving too far
                    // in the list of factors sorted by textpos
                    if(last_targetStart > targetStart) {
                        j = last_j;
                        byTextPos = byTextPosV[j];
                    }
                    
                    // find the closest factor, that has its last text position after
                    // or at the start of the factor we are trying to resolve
                    while(targetStart >= textPos(byTextPos) + factorLength(byTextPos)){
                        if(j + 1 < byTextPosV.size())
                            byTextPos = byTextPosV[++j];
                        else
                            byTextPos = sortByTextPos.max_value();
                    }
                        
                        
                    uint40_t textLen = factorLength(byTextPos);
                    uint40_t textStart = textPos(byTextPos);
                    uint40_t textEnd = textStart + textLen;
                    
                    
                    uint40_t prefixLen;                  
                    
                    // resolve prefix (or entire factor), if a prefix of the factor to resolve
                    // does not overlap with the other factor
                    if(targetStart < textStart) {
                        prefixLen = std::min(targetLen, uint40_t(textStart - targetStart));//targetLen - (std::max(targetEnd, textStart) - textStart);
                        uint40triple_t resolved = std::make_tuple(textPos(byTargetPos), targetPos(byTargetPos), prefixLen);
                        resolvedV.push_back(resolved);
                    }
                    // pointerjump prefix (or entire factor), if a prefix of the vector to resolve
                    // does overlap with the other factor
                    else {                       
                        auto posOffset = targetStart - textStart;
                        auto newTargetPos = targetPos(byTextPos) + posOffset;
                        prefixLen = std::min(targetLen, uint40_t(textEnd - targetStart));//targetLen - (std::max(targetEnd, textEnd) - textEnd);
                        uint40triple_t jumped = std::make_tuple(textPos(byTargetPos), newTargetPos, prefixLen);
                        
                        // add pointer-jumped factor to the end of the factor vector
                        // (this factor will not be considered in this scan, but in the next scan)
                        byTargetPosV.push_back(jumped);
                        ++byTargetPosResize;
                    }
                    
                    // if only a prefix of the current factor has been dealt with,
                    // calculate remaining factor and consider it next
                    if(prefixLen < targetLen) {
                        // manipulate the current factor directly in the factor vector
                        textPos(byTargetPos, textPos(byTargetPos) + prefixLen);
                        targetPos(byTargetPos, targetPos(byTargetPos) + prefixLen);
                        factorLength(byTargetPos, factorLength(byTargetPos) - prefixLen);
                        // by decrementing i, the manipulated factor will be considered next
                        --i;
                        // memorize the current position in the textpos sorted vector
                        // (this position will get restored, if the splitting of the
                        // current factor makes it necessary)
                        last_j = j;
                    } else {
                        // if the entire factor has been either resolved or pointer-jumped
                        // it is set to the maximum possible factor. this ensures, that it
                        // is in the backmost part of the sorted by target pos vector after
                        // the next sort. this way, it can be removed with a resize of the
                        // vector
                        byTargetPos = sortByTargetPos.max_value();
                        --byTargetPosResize;
                    }
                    
                    // memorize the last target start position to restore it later
                    // (if necessary)
                    last_targetStart = targetStart;
                }
                
                // sort the resolved factors by textposition
                stxxl::sort(resolvedV.begin(), resolvedV.end(), sortByTextPos, mb_ram*1024*1024);
                
                uint64_t failed = 0;
                uint64_t succeeded = 0;
                
                // restore the actual text from the resolved factors
                // (does this cause unstructured IO?)
                for(auto resolved : resolvedV) {
                    auto resolvedLen = factorLength(resolved);
                    auto resolvedTextPos = textPos(resolved) - 1;
                    auto resolvedTargetPos = targetPos(resolved) - 1;
                    for (unsigned i = 0; i < resolvedLen; i++) {
                        restoredText[resolvedTextPos + i] = restoredText[resolvedTargetPos + i];
                        if(restoredText[resolvedTextPos + i] == char(0) || restoredText[resolvedTextPos + i] == '?'){
							restoredText[resolvedTextPos + i]  = '?';
							failed++;
						} else {
							succeeded++;
						}
                    }
                    
                }
                resolvedV.clear();
                
                std::cout << "Failed:    " << failed << std::endl;
                std::cout << "Succeeded: " << succeeded << std::endl;
            }
            
            // print restored text, if it is short
            if(restoredText.size() < 500) {
                std::cout << "The restored text is: " << std::endl;
                for(auto character : restoredText) {
                    std::cout << character;
                }
                std::cout << std::endl;
            }
            
            // write restored text into output file
            std::ofstream outputFile;
            outputFile.open(outfilename);
            for (auto character : restoredText)
                outputFile << character;
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

