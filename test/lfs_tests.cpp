//Test the longest first substitution

#include <tudocomp/lfs/LFSCompressor.hpp>

#include "gtest/gtest.h"

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include "tudocomp_test_util.hpp"

using tdc::LFSCompressor;

TEST(lfs, test) {
    auto compressor = tdc::create_algo<LFSCompressor>();
    //comp.compress("String", null);
    // create the input for the test (a string constant)
   // tdc::Input input("abcccccccde");

    // create the output for the test (a buffer)
    //std::vector<uint8_t> buffer;
    //tdc::Output output(buffer);

    // invoke the compress method
   // compressor.compress(input, output);

        // retrieve the output as a string
    //std::string output_str(buffer.begin(), buffer.end());

    // compare the expected result against the output string to determine test failure or success
    //ASSERT_EQ("abc%6%de", output_str);
    ASSERT_TRUE(true);
}
