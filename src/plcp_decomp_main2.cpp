#include <algorithm>
#include <iostream>
#include <fstream>
#include <tudocomp_stat/StatPhaseStxxl.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>
#include <tudocomp/compressors/lcpcomp/decompress/PLCPDecompGenerator.hpp>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/sort.h>
#include <stxxl/bits/io/iostats.h>

//~ namespace tdc { namespace lcpcomp {
    

//~ using uint40_t = uint_t<40>;
//~ using uint_max_t = uint_t<64>;


//~ class PLCPDecomp {
 //~ private:
    //~ void push(uint64_t target, uint64_t length);
    //~ void prepare();
 //~ public:
    //~ void resolve();
//~ };

//~ template <typename unsigned_t>
//~ class PLCPDecompInternal : public PLCPDecomp {
    //~ friend class PLCPDecompGenerator;
  //~ private:
    //~ struct factor_t {
        //~ unsigned_t copyTo, copyFrom, length;
        
        //~ factor_t() {}        
        //~ factor_t(unsigned_t copyTo, unsigned_t copyFrom, unsigned_t length) {
            //~ this->copyTo = copyTo;
            //~ this->copyFrom = copyFrom;
            //~ this->length = length;
        //~ }
        
        //~ void removePrefix(unsigned_t prefixLength) {
            //~ copyTo += prefixLength;
            //~ copyFrom += prefixLength;
            //~ length -= prefixLength;
        //~ }
    //~ };
    
    //~ const unsigned_t min_unsigned = std::numeric_limits<unsigned_t>::min();
    //~ const unsigned_t max_unsigned = std::numeric_limits<unsigned_t>::max();
    //~ const factor_t min_factor = factor_t(min_unsigned, min_unsigned, min_unsigned);
    //~ const factor_t max_factor = factor_t(max_unsigned, max_unsigned, max_unsigned);

    //~ struct factor_compare_by_copyTo_t {
        //~ bool operator()(const factor_t &a, const factor_t &b) const 
        //~ { return a.copyTo < b.copyTo; }
        //~ factor_t min_value() const { return min_factor; }
        //~ factor_t max_value() const { return max_factor; }
    //~ };
    
    //~ struct factor_compare_by_copyFrom_t {
        //~ bool operator()(const factor_t &a, const factor_t &b) const 
        //~ { return a.copyFrom < b.copyFrom; }
        //~ factor_t min_value() const { return min_factor; }
        //~ factor_t max_value() const { return max_factor; }
    //~ };
 
    //~ typedef typename stxxl::VECTOR_GENERATOR<factor_t>::result vector_of_factor_t;
    //~ typedef typename stxxl::VECTOR_GENERATOR<char>::result vector_of_char_t;
 
    //~ len_t bytes_memory;
    
    //~ vector_of_char_t textBuffer;
    //~ vector_of_factor_t byCopyToV;
    //~ vector_of_factor_t byCopyFromV;
    //~ vector_of_factor_t resolvedV;
    
    //~ unsigned_t pushNextPos = 0;
    
    //~ PLCPDecompInternal(unsigned_t nReferences = 0, unsigned_t nLiterals = 0);

    //~ void push(unsigned_t target, unsigned_t length);
    
 //~ public:
 
    //~ void resolve();
//~ };

//~ template <typename unsigned_t>
//~ PLCPDecompInternal<unsigned_t>::PLCPDecompInternal(
        //~ unsigned_t nReferences, 
        //~ unsigned_t nLiterals) {
    //~ byCopyToV.reserve(nReferences);
    //~ textBuffer.resize(nLiterals + nReferences);
//~ }


//~ template <typename unsigned_t>
//~ void PLCPDecompInternal<unsigned_t>::push(
        //~ unsigned_t target, 
        //~ unsigned_t length) {
    //~ if(length == unsigned_t(0)) {
        //~ textBuffer[pushNextPos] = char(target);
        //~ pushNextPos++;
    //~ } 
    //~ else {
        //~ factor_t factor(pushNextPos + 1, target + 1, length);
        //~ byCopyToV.push_back(factor);
        //~ pushNextPos += length;
    //~ }
//~ }

        
//~ template <typename unsigned_t>
//~ void PLCPDecompInternal<unsigned_t>::resolve() {
        
//~ }


//~ class PLCPDecompGenerator {
 //~ private: 
    //~ inline static uint40_t swapBytes(uint40_t value);
    
 //~ public:
    //~ static PLCPDecomp getInstance(const std::string& infilename, len_t megabytes_memory);
//~ };

//~ PLCPDecomp PLCPDecompGenerator::getInstance(const std::string& infilename, len_t megabytes_memory){

    //~ std::cout << "Generating PLCP decompressor..." << std::endl;

    //~ IntegerFileArray<uint40_t> text (infilename.c_str());

    //~ const size_t nFactors = text.size() / 2;
    //~ uint40_t nReferences = 0;
    //~ uint40_t nLiterals = 0;

    //~ // count literals and number of characters contained in references
    //~ for (size_t i = 0; i < nFactors; i++) {
        //~ uint40_t target = swapBytes(text[2 * i]);
        //~ uint40_t len = swapBytes(text[2 * i + 1]);
        //~ if(len == uint40_t(0)) nLiterals += 1;
        //~ else nReferences += len;
    //~ }
    
    //~ int bitsPerInt = 1 + (int) std::log2((uint64_t)(nReferences + nLiterals + 1));
    //~ int bytesPerInt = (bitsPerInt + 7) / 8;
    
    //~ std::cout << "Factors:              " << nFactors << std::endl;
    //~ std::cout << "Literal characters:   " << nLiterals << std::endl;
    //~ std::cout << "Reference characters: " << nReferences << std::endl;
    //~ std::cout << "Total characters:     " << nReferences + nLiterals << std::endl;
    //~ std::cout << "Bits per integer:     " << bitsPerInt << std::endl;
    //~ std::cout << "Bytes per integer:    " << bytesPerInt << std::endl;

    //~ PLCPDecomp decompressor;

    //~ if(bytesPerInt == 1) decompressor = PLCPDecompInternal<uint_t<8>>();
    //~ else if(bytesPerInt == 2) decompressor = PLCPDecompInternal<uint_t<16>>();
    //~ else if(bytesPerInt == 3) decompressor = PLCPDecompInternal<uint_t<24>>();
    //~ else if(bytesPerInt == 4) decompressor = PLCPDecompInternal<uint_t<32>>();
    //~ else if(bytesPerInt == 5) decompressor = PLCPDecompInternal<uint_t<40>>();
    //~ else decompressor = PLCPDecompInternal<uint_t<64>>();
    
    //~ std::cout << "Reserving space for vector of unresolved factors..." << std::endl;
    //~ auto &byCopyToV = decompressor;
    //~ byCopyToV.reserve(nReferences);
    
    
    //~ return decompressor;
//~ }

//~ template<int bytesPerUInt>
//~ PLCPDecomp &PLCPDecompGenerator::getInstanceInternal(const std::string& infilename, len_t megabytes_memory){
    //~ typedef uint_t<bytesPerUInt * 8> uint_t;
    
    //~ std::cout << "Reserving space for vector of unresolved factors..." << std::endl;
    //~ typename PLCPDecompInternal<uint_t>::vector_of_factor_t byCopyToV;
    //~ byCopyToV.reserve(nReferences);
    
    //~ std::cout << "Reserving space for vector of resolved characters..." << std::endl;
    //~ typename PLCPDecompInternal<uint_t>::vector_of_char_t restoredText;
    //~ restoredText.resize(nLiterals + nReferences);    
    
    //~ std::cout << "Parsing unresolved factors and literals..." << std::endl;
    //~ std::cout << "    (fill vector of unresolved factors in copyTo order)" << std::endl;
    //~ std::cout << "    (fill vector of resolved characters with literals)" << std::endl;
    
    //~ // everything here is 1-based
    //~ // (necessary for stxxl sort)
    //~ uint40_t textPosition = 1;
    //~ for (size_t i = 0; i < nFactors; i++) {
        //~ // read current factor
        //~ uint40_t target = swapBytes(text[i * 2]) + 1;
        //~ uint40_t len = swapBytes(text[i * 2 + 1]);
        //~ if(len == uint40_t(0)) {
            //~ // restore literal factors immediatly
            //~ // (naturally in textpos order -> single scan)
            //~ restoredText[textPosition - 1] = char(target - 1);
            //~ textPosition++;
        //~ } 
        //~ else {
            //~ // fill one of the factor vectors
            //~ // (other one will get copied)
            //~ auto reference = factor_t<>(textPosition, target, len);
            //~ byCopyToV.push_back(reference);
            //~ textPosition += len;
        //~ }
    //~ }
    
    //~ return PLCPDecompInternal<uint_t>(restoredText, byCopyToV, megabytes_memory);
//~ }


//~ uint40_t PLCPDecompGenerator::swapBytes(uint40_t value) {
    //~ uint40_t result = 0;
    //~ result |= (value & 0xFF) << 32;
    //~ result |= (value & 0xFF00) << 16;
    //~ result |= (value & 0xFF0000);
    //~ result |= (value & 0xFF000000) >> 16;
    //~ result |= (value & (uint40_t)0xFF00000000) >> 32;
    //~ return result;
//~ }



//~ }}//ns


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
    
    tdc::StatPhaseStxxlConfig statsCfg(false);
    statsCfg.parallelTime_combined = true;
    statsCfg.waitTime_combined = true;
    statsCfg.numberOfBytes_read = true;
    statsCfg.numberOfBytes_write = true;
    statsCfg = tdc::StatPhaseStxxlConfig();
    tdc::StatPhaseStxxl::enable(statsCfg);
    tdc::StatPhase root("Root");
    
    root.to_json();
    root.to_json();
    root.to_json();

    const std::string infile { argv[1] };
    const std::string outfile { argv[2] };
    if(!file_exist(infile)) {
        std::cerr << "Infile " << infile << " does not exist" << std::endl;
        return 1;
    }

    const tdc::len_t mb_ram = (argc >= 3) ? std::stoi(argv[3]) : 512;

    tdc::lcpcomp::IntegerFileArray<tdc::uint_t<40>> compressed (infile.c_str());

    auto textBuffer = tdc::lcpcomp::PLCPDecompGenerator::decompress(compressed, mb_ram);
    
    std::ofstream outputFile;
    outputFile.open(outfile);
    for (auto character : textBuffer)
        outputFile << character;
    outputFile.close();

    auto algorithm_stats = root.to_json();

    tdc::json::Object meta;
    meta.set("title", "TITLE");
    meta.set("startTime", "STARTTIME");

    meta.set("config", "NONE");
    meta.set("input", "FILENAME");;
    meta.set("inputSize", "INPUTSIZE");
    meta.set("output", "NONE");
    meta.set("outputSize", "OUTPUTSIZE");
    meta.set("rate", "NONE");

    tdc::json::Object stats;
    stats.set("meta", meta);
    stats.set("data", algorithm_stats);

    std::cout << "data for http://tudocomp.org/charter" << std::endl;
    stats.str(std::cout);
    std::cout << std::endl;

    return 0;
}

