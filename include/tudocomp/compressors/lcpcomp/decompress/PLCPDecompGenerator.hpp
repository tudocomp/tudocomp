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

class PLCPDecompGenerator {
 public:
    typedef typename stxxl::VECTOR_GENERATOR<char>::result stxxl_vector_of_char_t;
 private:
    template<typename unsigned_t>
    inline static unsigned_t swapBytes(uint_t<40> value);
    
    template<unsigned bitsPerUInt>
    static stxxl_vector_of_char_t decompressInternal(
        typename PLCPDecomp<bitsPerUInt>::vector_of_char_t &textBuffer,
        typename PLCPDecomp<bitsPerUInt>::vector_of_ref_initial_t &factors, 
        len_t bytes_memory);
    
 public:
        
    static stxxl_vector_of_char_t decompress(
        IntegerFileArray<uint_t<40>> &factors, 
        len_t megabytes_memory);
};

PLCPDecompGenerator::stxxl_vector_of_char_t PLCPDecompGenerator::decompress
        (IntegerFileArray<uint_t<40>> &factors, len_t megabytes_memory){
    
    using compressor_t = PLCPDecomp<40>;
    
    std::cout << "Preparing decompression..." << std::endl;

    len_t nBytesMemory = megabytes_memory * 1000 * 1000;
    uint_t<40> nFactors = factors.size() / 2;
    uint_t<40> nReferences = 0;
    uint_t<40> nLiterals = 0;
    
    compressor_t::vector_of_ref_initial_t factorsForCompressor;
    compressor_t::vector_of_ref_writer_initial_t factorWriter(factorsForCompressor);
    
    compressor_t::vector_of_char_t textBuffer;
    compressor_t::vector_of_char_writer_t textWriter(textBuffer);

    std::cout << "    Filling initial vectors..." << std::endl;

    uint_t<40> textPosition = 1;
    for (uint_t<40> i = 0; i < nFactors; i++) {
       
        if (i % 500000 == 0) {
            std::cout << "        Factor " << i << " of " << nFactors << "... " << std::endl;
        }
        
        //~ uint_t<40> target(swapBytes<uint_t<40>>(factors[2 * i]) + 1);
        //~ uint_t<40> len(swapBytes<uint_t<40>>(factors[2 * i + 1]));
        uint_t<40> target(factors[i * 2] + 1);
        uint_t<40> len(factors[i * 2 + 1]);
        
        if(len == uint_t<40>(0)) {
            textWriter << char(target - 1);
            textPosition++;
            nLiterals++;
        } 
        else {
            factorWriter << compressor_t::ref_initial_t(textPosition, target, len);
            for (uint_t<40> j = 0; j < len; j++) textWriter << '\0';
            textPosition += len;
            nReferences += len;
        }
    }
    factorWriter.finish();
    textWriter.finish();
    
    unsigned bitsPerInt = 1 + (int) std::log2((uint64_t)(nReferences + nLiterals + 1));
    
    std::cout << "    Factors:                " << nFactors << std::endl;
    std::cout << "    Literal characters:     " << nLiterals << std::endl;
    std::cout << "    Reference characters:   " << nReferences << std::endl;
    std::cout << "    Total characters:       " << nReferences + nLiterals << std::endl;
    std::cout << "    Bits per integer:       " << bitsPerInt << std::endl;
    std::cout << "    Allowed memory (bytes): " << nBytesMemory << std::endl;
    
    //~ if(specs.bitsPerInt <= 4) return decompressInternal<uint_t<4>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 8) return decompressInternal<uint_t<8>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 12) return decompressInternal<uint_t<12>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 16) return decompressInternal<uint_t<16>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 20) return decompressInternal<uint_t<20>>(factors, specs);
    //~ else if(specs.bitsPerInt <= 24) return decompressInternal<40, 40>(textBuffer, factorsForCompressor, nBytesMemory);
    //~ else if(specs.bitsPerInt <= 28) return decompressInternal<40, 40>(textBuffer, factorsForCompressor, nBytesMemory);
    //~ else 
    if(bitsPerInt <= 32) return decompressInternal<32>(textBuffer, factorsForCompressor, nBytesMemory);
    //~ else if(specs.bitsPerInt <= 36) return decompressInternal<40, 40>(textBuffer, factorsForCompressor, nBytesMemory);
    else /*if(specs.bitsPerInt <= 40)*/ return decompressInternal<40>(textBuffer, factorsForCompressor, nBytesMemory);
}

template<unsigned bitsPerUInt>
PLCPDecompGenerator::stxxl_vector_of_char_t PLCPDecompGenerator::decompressInternal(
        typename PLCPDecomp<bitsPerUInt>::vector_of_char_t &textBuffer,
        typename PLCPDecomp<bitsPerUInt>::vector_of_ref_initial_t &factors, 
        len_t bytes_memory) {

    PLCPDecomp<bitsPerUInt> compressor(textBuffer, factors, bytes_memory);
    return compressor.decompress();
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
