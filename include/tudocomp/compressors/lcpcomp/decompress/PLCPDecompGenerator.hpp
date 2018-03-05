#include <algorithm>
#include <iostream>
#include <fstream>
#include <tudocomp_stat/StatPhaseStxxl.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/compressors/lcpcomp/decompress/PLCPDecomp.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/sort.h>
#include <stxxl/bits/io/iostats.h>

namespace tdc { namespace lcpcomp {
    
struct PLCPDecompSpecs {
    len_t mbMem;
    uint_t<40> nFactors, nLiterals, nReferences, bitsPerInt;
};

class PLCPDecompGenerator {
 public:
    typedef typename stxxl::VECTOR_GENERATOR<char>::result stxxl_vector_of_char_t;
 private:
    template<typename unsigned_t>
    inline static unsigned_t swapBytes(uint_t<40> value);
    
    template<typename unsigned_t>
    static stxxl_vector_of_char_t decompressInternal(
        IntegerFileArray<uint_t<40>> &factors, 
        PLCPDecompSpecs specs);
    
 public:
 
    static PLCPDecompSpecs getSpecs(
        IntegerFileArray<uint_t<40>> &factors, 
        len_t megabytes_memory);
        
    static stxxl_vector_of_char_t decompress(
        IntegerFileArray<uint_t<40>> &factors, 
        len_t megabytes_memory);
};



PLCPDecompSpecs PLCPDecompGenerator::getSpecs(IntegerFileArray<uint_t<40>> &factors, len_t megabytes_memory){

    std::cout << "Calculating PLCP decomp specifications. Memory for STXXL (in MB): " << megabytes_memory << std::endl;

    PLCPDecompSpecs result;
    result.mbMem = megabytes_memory;
    result.nFactors = factors.size() / 2;
    result.nReferences = 0;
    result.nLiterals = 0;

    // count literals and number of characters contained in references
    for (uint_t<40> i = 0; i < result.nFactors; i++) {
        uint_t<40> len = swapBytes<uint_t<40>>(factors[2 * i + 1]);
        if(len == uint_t<40>(0)) result.nLiterals += 1;
        else result.nReferences += len;
    }
    
    result.bitsPerInt = 1 + (int) std::log2((uint64_t)(result.nReferences + result.nLiterals + 1));
    
    std::cout << "Factors:              " << result.nFactors << std::endl;
    std::cout << "Literal characters:   " << result.nLiterals << std::endl;
    std::cout << "Reference characters: " << result.nReferences << std::endl;
    std::cout << "Total characters:     " << result.nReferences + result.nLiterals << std::endl;
    std::cout << "Bits per integer:     " << result.bitsPerInt << std::endl;

    return result;
}

PLCPDecompGenerator::stxxl_vector_of_char_t PLCPDecompGenerator::decompress
        (IntegerFileArray<uint_t<40>> &factors, len_t megabytes_memory){

    PLCPDecompSpecs specs = getSpecs(factors, megabytes_memory);
    
    //~ if(specs.bitsPerInt <= 4) return decompressInternal<uint_t<4>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 8) return decompressInternal<uint_t<8>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 12) return decompressInternal<uint_t<12>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 16) return decompressInternal<uint_t<16>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 20) return decompressInternal<uint_t<20>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 24) return decompressInternal<uint_t<24>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 28) return decompressInternal<uint_t<28>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 32) return decompressInternal<uint_t<32>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 36) return decompressInternal<uint_t<36>>(factors, specs);
    /*else /*if(specs.bitsPerInt <= 40)*/ return decompressInternal<uint_t<40>>(factors, specs);
}

template<typename unsigned_t>
PLCPDecompGenerator::stxxl_vector_of_char_t PLCPDecompGenerator::decompressInternal
        (IntegerFileArray<uint_t<40>> &factors, PLCPDecompSpecs specs){

    std::cout << "\nStarting decompression..." << std::endl;
    
    PLCPDecomp<unsigned_t> decompressor;
    
    std::cout << "Reserving space for vectors..." << std::endl;
    decompressor.byCopyToV->reserve(specs.nReferences);
    decompressor.byCopyToV_new->reserve(specs.nReferences);
    decompressor.byCopyFromV->reserve(specs.nReferences);
    decompressor.textBuffer.resize(specs.nReferences + specs.nLiterals);
    
    decompressor.bytes_memory = specs.mbMem * 1024 * 1024;
    
    std::cout << "Parsing unresolved factors and literals..." << std::endl;
    std::cout << "    (fill vector of unresolved factors in copyTo order)" << std::endl;
    std::cout << "    (fill vector of restored characters with literals)" << std::endl;
    
    // everything here is 1-based
    // (necessary for stxxl sort)
    unsigned_t textPosition = 1;
    for (uint_t<40> i = 0; i < specs.nFactors; i++) {
        // read current factor
        unsigned_t target(swapBytes<unsigned_t>(factors[i * 2]) + 1);
        unsigned_t len(swapBytes<unsigned_t>(factors[i * 2 + 1]));
        if(len == unsigned_t(0)) {
            // restore literal factors immediatly
            // (naturally in textpos order -> single scan)
            decompressor.textBuffer[textPosition - 1] = char(target - 1);
            textPosition++;
        } 
        else {
            // fill one of the factor vectors
            // (other one will get copied)
            auto reference = reference_t<unsigned_t>(textPosition, target, len);
            decompressor.byCopyToV->push_back(reference);
            textPosition += len;
        }
    }
    
    return decompressor.decompress();
}

template<typename unsigned_t>
unsigned_t PLCPDecompGenerator::swapBytes(uint_t<40> value) {
    unsigned_t result = 0;
    result |= (value & 0xFF) << 32;
    result |= (value & 0xFF00) << 16;
    result |= (value & 0xFF0000);
    result |= (value & 0xFF000000) >> 16;
    result |= (value & (uint_t<40>)0xFF00000000) >> 32;
    return result;
}

}} //ns
