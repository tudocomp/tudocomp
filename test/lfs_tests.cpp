//Test the longest first substitution

#include <tudocomp/compressors/lfs/LFSCompressor.hpp>

#include "gtest/gtest.h"

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include "tudocomp_test_util.hpp"

using tdc::LFSCompressor;
using namespace tdc;


template<typename coder_t>
void run_coder_test(const std::string compression_string) {
    auto c = create_algo<LFSCompressor<coder_t>>();

    std::string compressed;
    // compress
    {
        Input dummy_input(compression_string);
        std::stringstream stm;
        Output output = Output::from_stream(stm);

        c.compress(dummy_input, output);
        compressed=stm.str();
        DLOG(INFO) << "compressed:";
        DLOG(INFO) << compressed;
    }
    // decompress
    {
        auto c = create_algo<LFSCompressor<coder_t>>();

        Input file_input(compressed);
        std::stringstream stm;
        Output dummy_output = Output::from_stream(stm);

        c.decompress(file_input, dummy_output);
        DLOG(INFO) << "decompressed:";
        DLOG(INFO) << stm.str();

    }
}


template<typename coder_t>
void run_coder_test_to_file(const std::string filename, const std::string compression_string) {
    auto c = create_algo<LFSCompressor<coder_t>>();

    // compress
    {
        Input dummy_input(compression_string);
        Output file_output(filename, true);

        c.compress(dummy_input, file_output);
    }
    // decompress
    {
        auto c = create_algo<LFSCompressor<coder_t>>();

        Input file_input(Input::Path{filename});
        std::stringstream stm;
        Output dummy_output = Output::from_stream(stm);

        c.decompress(file_input, dummy_output);
        DLOG(INFO) << "decompressed:";
        DLOG(INFO) << stm.str();
    }
}

template<typename coder_t>
void compress_and_decompress_file(const std::string filename) {
    auto c = create_algo<LFSCompressor<coder_t>>();

    // compress
    {
        Input file_input(Input::Path{filename});
        Output file_output(filename+".lfs", true);

        c.compress(file_input, file_output);
    }
    // decompress
    {
        auto c = create_algo<LFSCompressor<coder_t>>();

        Input file_input(Input::Path{filename+".lfs"});
        Output file_output(filename+".decomp", true);

        c.decompress(file_input, file_output);
        DLOG(INFO) << "decompressed";

    }
}


TEST(lfs, as_stream_aba){

     run_coder_test<BitOptimalCoder>("abaaabbababb$");

     run_coder_test<ASCIICoder>("abaaabbababb$");
}


TEST(lfs, as_stream_mis){
    // run_coder_test<BitOptimalCoder>("mississippi$");

   // run_coder_test<ASCIICoder>("mississippi$");
}

TEST(lfs, as_file_aba){

    // run_coder_test_to_file<BitOptimalCoder>("out2.bits", "abaaabbababb$");

   // run_coder_test_to_file<ASCIICoder>("out2.ascii", "abaaabbababb$");
}

TEST(lfs, as_file_mis){
    // run_coder_test_to_file<BitOptimalCoder>("out.bits", "mississippi$");

  //  run_coder_test_to_file<ASCIICoder>("out.ascii", "mississippi$");
}

TEST(lfs, large_file){
    compress_and_decompress_file<BitOptimalCoder>("sources.1MB");
}


