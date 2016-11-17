//Test the longest first substitution

#include <tudocomp/lfs/LFSCompressor.hpp>

//#include <tudocomp/example/ExampleCompressor.hpp>

#include "gtest/gtest.h"

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include "tudocomp_test_util.hpp"

using tdc::LFSCompressor;

TEST(lfs, test) {

    std::string comp_result;
    auto compressor = tdc::create_algo<LFSCompressor>();

    tdc::Input input("abaaabbababb$");

    DLOG(INFO) << "input:";
    DLOG(INFO) << "abaaabbababb$";


    std::stringstream stm;
    Output output = Output::from_stream(stm);





    // invoke the compress method
    compressor.compress(input, output);

    comp_result = stm.str();
    ASSERT_EQ("\\Baa\\A\\B\\A$\\$abb\\$ab\\$", comp_result);

    DLOG(INFO) << "encoded:";
    DLOG(INFO) << comp_result;

    std::stringstream stm2;
    Output output2 = Output::from_stream(stm2);

    tdc::Input input_decompress(comp_result);

    compressor.decompress(input_decompress, output2);

    std::string decode_result = stm2.str();

    // compare the expected result against the output string to determine test failure or success
    //ASSERT_EQ("abc%6%de", output_str);
    ASSERT_EQ("abaaabbababb$", decode_result);
    //ASSERT_TRUE(true);
}

TEST(lfs, test2) {

    std::string comp_result;
    auto compressor = tdc::create_algo<LFSCompressor>();

    // create the input for the test (a string constant)
    tdc::Input input("mississippi$");
    DLOG(INFO) << "input:";
    DLOG(INFO) << "mississippi$";
    // create the output for the test (a buffer)


    std::stringstream stm;
    Output output = Output::from_stream(stm);





    // invoke the compress method
    compressor.compress(input, output);

    comp_result = stm.str();
    DLOG(INFO) << "encoded:";
    DLOG(INFO) << comp_result << std::endl;

    tdc::Input input_decompress(comp_result);

    std::stringstream stm2;
    Output output2 = Output::from_stream(stm2);


    compressor.decompress(input_decompress, output2);
    std::string decode_result = stm2.str();
    // compare the expected result against the output string to determine test failure or success
    ASSERT_TRUE(true);

    ASSERT_EQ("mississippi$", decode_result);
}


TEST(lfs, roundtrip1) {
    test::roundtrip<LFSCompressor>("abaaabbababb", "\\Baa\\A\\B\\A\\$abb\\$ab\\$");
}
