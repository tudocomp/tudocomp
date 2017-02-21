//Test the longest first substitution

#include <tudocomp/compressors/lfs/ESALFSCompressor.hpp>
#include <tudocomp/compressors/lfs/STLFSCompressor.hpp>

#include <tudocomp/compressors/lfs/LFSCompressor.hpp>
#include <tudocomp/compressors/lfs/STStrategy.hpp>

#include <tudocomp/compressors/lfs/ESAStrategy.hpp>

#include "gtest/gtest.h"

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include "test/util.hpp"

using tdc::ESALFSCompressor;
using namespace tdc;


template<typename lit_coder_t, typename len_coder_t>
void run_coder_test_esa(const std::string compression_string) {
    auto c = create_algo<ESALFSCompressor<lit_coder_t, len_coder_t>>();

    std::string compressed;
    // compress
    {
        test::TestInput dummy_input = test::compress_input(compression_string);
        test::TestOutput output = test::compress_output();

        c.compress(dummy_input, output);
        compressed=output.result();

    }
    // decompress
    {

        test::TestInput input = test::decompress_input(compressed);
        test::TestOutput output = test::decompress_output();

        c.decompress(input, output);

        compressed=output.result();
    }
    ASSERT_EQ(compression_string, compressed);
}

template<typename lit_coder_t, typename len_coder_t>
void run_coder_test_st(const std::string compression_string) {
    auto c = create_algo<STLFSCompressor<lit_coder_t, len_coder_t>>();

    std::string compressed;
    // compress
    {
        test::TestInput dummy_input = test::compress_input(compression_string);
        test::TestOutput output = test::compress_output();

        c.compress(dummy_input, output);
        compressed=output.result();

    }
    // decompress
    {

        test::TestInput input = test::decompress_input(compressed);
        test::TestOutput output = test::decompress_output();

        c.decompress(input, output);

        compressed=output.result();
    }
    ASSERT_EQ(compression_string, compressed);
}


template<typename lit_coder_t, typename len_coder_t>
void run_coder_test_to_file(const std::string filename, const std::string compression_string) {
    auto c = create_algo<ESALFSCompressor<lit_coder_t, len_coder_t>>();

    // compress
    {
        test::TestInput dummy_input = test::compress_input(compression_string);
        Output file_output(filename, true);

        c.compress(dummy_input, file_output);
    }
    // decompress
    {
        //auto c = create_algo<LFSCompressor<lit_coder_t, len_coder_t>>();

        test::TestInput file_input = test::decompress_input_file(filename);

       // std::stringstream stm;
       // Output dummy_output = Output::from_stream(stm);
        test::TestOutput output = test::compress_output();

        c.decompress(file_input, output);
    }
}

template<typename lit_coder_t, typename len_coder_t>
void compress_and_decompress_file(const std::string filename, std::string extension) {
    auto c = create_algo<ESALFSCompressor<lit_coder_t, len_coder_t> >();

    // compress
    {
        test::TestInput file_input= test::compress_input_file(filename);
        //file_input.escape_and_terminate();
        Output file_output(filename+"."+extension, true);

        c.compress(file_input, file_output);
    }
    // decompress
    {
        //auto c = create_algo<LFSCompressor<lit_coder_t, len_coder_t>>();

        test::TestInput file_input = test::decompress_input_file(filename+"."+extension);
        Output file_output(filename+".decomp", true);

        c.decompress(file_input, file_output);
        DLOG(INFO) << "decompressed";

    }
}

template<typename lit_coder_t, typename len_coder_t>
void compress_and_decompress_file_st(const std::string filename, std::string extension) {
    auto c = create_algo<STLFSCompressor<lit_coder_t, len_coder_t> >();

    // compress
    {
        test::TestInput file_input= test::compress_input_file(filename);
        //file_input.escape_and_terminate();
        Output file_output(filename+"."+extension, true);

        c.compress(file_input, file_output);
    }
    // decompress
    {
        //auto c = create_algo<LFSCompressor<lit_coder_t, len_coder_t>>();

        test::TestInput file_input = test::decompress_input_file(filename+"."+extension);
        Output file_output(filename+".decomp", true);

        c.decompress(file_input, file_output);
        DLOG(INFO) << "decompressed";

    }
}

TEST(stlfs, as_stream_aba){
  //  run_coder_test_st<BitCoder,EliasGammaCoder>("abaaabbababb$");
}

TEST(stlfs, as_stream_mis){
  //  run_coder_test_st<BitCoder,EliasGammaCoder>("mississippi$");
}

TEST(stlfs, file_english_1mb_eg){
   // compress_and_decompress_file_st<BitCoder, BitCoder>("english.1MB", "stlfs");
}


TEST(lfs, st_strat){
        auto c = create_algo<lfs::LFSCompressor< lfs::STStrategy<> > >();

        // compress
        std::string compression_string = "abaaabbababb$";
        std::string compressed;
        // compress
        {
            test::TestInput dummy_input = test::compress_input(compression_string);
            test::TestOutput output = test::compress_output();

            c.compress(dummy_input, output);
            compressed=output.result();

        }
        // decompress
        {

            test::TestInput input = test::decompress_input(compressed);
            test::TestOutput output = test::decompress_output();

            c.decompress(input, output);

            compressed=output.result();
        }
        ASSERT_EQ(compression_string, compressed);
}

TEST(lfs, esa_strat){
        auto c = create_algo<lfs::LFSCompressor< lfs::ESAStrategy<> > >();

        // compress
        std::string compression_string = "abaaabbababb$";
        std::string compressed;
        // compress
        {
            test::TestInput dummy_input = test::compress_input(compression_string);
            test::TestOutput output = test::compress_output();

            c.compress(dummy_input, output);
            compressed=output.result();

        }
        // decompress
        {

            test::TestInput input = test::decompress_input(compressed);
            test::TestOutput output = test::decompress_output();

            c.decompress(input, output);

            compressed=output.result();
        }
        ASSERT_EQ(compression_string, compressed);
}


TEST(lfs, as_stream_aba){

     //run_coder_test<BitCoder,EliasGammaCoder>("abaaabbababb$");

    // run_coder_test<ASCIICoder,ASCIICoder>("abaaabbababb$");
}


TEST(lfs, as_stream_mis){
     //run_coder_test<BitCoder,EliasGammaCoder>("mississippi$");

   // run_coder_test<ASCIICoder,EliasGammaCoder>("mississippi$");
}

TEST(lfs, as_file_aba){

    // run_coder_test_to_file<BitOptimalCoder>("out2.bits", "abaaabbababb$");

   // run_coder_test_to_file<ASCIICoder>("out2.ascii", "abaaabbababb$");
}

TEST(lfs, as_file_mis){
    // run_coder_test_to_file<BitOptimalCoder>("out.bits", "mississippi$");

  //  run_coder_test_to_file<ASCIICoder>("out.ascii", "mississippi$");
}

TEST(lfs, file_sources_1mb_eg){
    //compress_and_decompress_file<BitCoder, BitCoder>("sources.1MB", "lfs.bc");
}

TEST(lfs, file_sources_10mb_eg){
    //compress_and_decompress_file<BitCoder, BitCoder>("sources.10MB", "lfs.bc");
}

TEST(lfs, file_english_1mb){
    //compress_and_decompress_file<BitCoder, BitCoder>("english.1MB", "esalfs.bc");
}

TEST(lfs, file_english_10mb){
    //compress_and_decompress_file<BitCoder, BitCoder>("english.10MB", "lfs.bc");
}


