//Test the longest first substitution

#include <tudocomp/compressors/lfs/LFSCompressor.hpp>

//#include <tudocomp/example/ExampleCompressor.hpp>

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


TEST(lfs, as_stream){
     run_coder_test<BitOptimalCoder>("mississippi$");

    run_coder_test<ASCIICoder>("mississippi$");

     run_coder_test<BitOptimalCoder>("abaaabbababb$");

    run_coder_test<ASCIICoder>("abaaabbababb$");
}

TEST(lfs, as_file){
     run_coder_test_to_file<BitOptimalCoder>("out.bits", "mississippi$");

    run_coder_test_to_file<ASCIICoder>("out.ascii", "mississippi$");

     run_coder_test_to_file<BitOptimalCoder>("out2.bits", "abaaabbababb$");

    run_coder_test_to_file<ASCIICoder>("out2.ascii", "abaaabbababb$");
}

//doesnt work anymore
/*TEST(lfs, roundtrip1) {
    test::roundtrip<LFSCompressor<ASCIICoder>>("abaaabbababb", "\\Baa\\A\\B\\A\0\\$abb\\$ab\\$");
}*/

