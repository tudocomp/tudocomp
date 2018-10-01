#include <algorithm>
#include <iostream>
#include <fstream>
#include <tudocomp_stat/StatPhaseStxxl.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/compressors/lcpcomp/decompress/PLCPDecomp.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/sort.h>
#include <stxxl/bits/io/iostats.h>

namespace tdc { namespace lcpcomp {

class PLCPDecompGenerator {
 private:
    template<typename unsigned_t>
    inline static unsigned_t swapBytes(uint_t<40> value);

    template<unsigned bitsPerUInt>
    static vector_of_char_t decompressInternal(
        vector_of_char_t &textBuffer,
        vector_of_ref_initial_t &factors,
        uint64_t bytesMemory);

 public:

    static vector_of_char_t decompress(
        vector_of_upair_initial_t &factors,
        uint64_t mebiBytesMemory);
};

vector_of_char_t PLCPDecompGenerator::decompress
        (vector_of_upair_initial_t &factors, uint64_t mebiBytesMemory){

    using compressor_t = PLCPDecomp<40>;

    std::cout << "Preparing decompression..." << std::endl;

    uint64_t nBytesMemory = mebiBytesMemory * 1024 * 1024;
    uint_t<40> nFactors = factors.size();
    uint_t<40> nReferences = 0;
    uint_t<40> nLiterals = 0;

    vector_of_ref_initial_t factorsForCompressor;
    vector_of_ref_writer_initial_t factorWriter(factorsForCompressor);

    vector_of_char_t textBuffer = compressor_t::getEmptyTextBuffer(nBytesMemory);
    vector_of_char_writer_t textWriter(textBuffer);

    std::cout << "    Filling initial vectors..." << std::endl;

    vector_of_upair_reader_initial_t inFactors(factors);
    auto inFactorsIt = inFactors.begin();

    uint_t<40> textPosition = 1;
    for (uint_t<40> i = 0; i < nFactors; i++) {

        if (i % 500000 == 0) {
            std::cout << "        Factor " << i << " of " << nFactors << "... " << std::endl;
        }

        uint_t<40> target = inFactorsIt->first + 1;
        uint_t<40> len =    inFactorsIt->second;
        ++inFactorsIt;

        if(len == uint_t<40>(0)) {
            textWriter << char(target - 1);
            textPosition++;
            nLiterals++;
        }
        else {
            factorWriter << ref_initial_t(textPosition, target, len);
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

    //~ if(bitsPerInt <= 24) return decompressInternal<24>(textBuffer, factorsForCompressor, nBytesMemory);
    //~ else if(bitsPerInt <= 32) return decompressInternal<32>(textBuffer, factorsForCompressor, nBytesMemory);
    /*else if(specs.bitsPerInt <= 40)*/ return decompressInternal<40>(textBuffer, factorsForCompressor, nBytesMemory);
}

template<unsigned bitsPerUInt>
vector_of_char_t PLCPDecompGenerator::decompressInternal(
        vector_of_char_t &textBuffer,
        vector_of_ref_initial_t &factors,
        uint64_t bytesMemory) {

    PLCPDecomp<bitsPerUInt> compressor(textBuffer, factors, bytesMemory);
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
