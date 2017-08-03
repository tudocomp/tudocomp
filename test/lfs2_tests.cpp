//Test the longest first substitution, type 2


#include <tudocomp/compressors/lfs/LFS2Compressor.hpp>
#include "gtest/gtest.h"

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include "test/util.hpp"

#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/EliasGammaCoder.hpp>
#include <tudocomp/coders/EliasDeltaCoder.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>

using namespace tdc;
using namespace lfs;




void run_comp(std::string compression_string) {
    auto c = create_algo<tdc::lfs::LFS2Compressor<> >();

    std::string compressed;
    // compress
    {
        test::TestInput dummy_input = test::compress_input(compression_string);
        test::TestOutput output = test::compress_output();

        c.compress(dummy_input, output);
        compressed=output.result();

    }
    // decompress
    DLOG(INFO)<<"compressed: " << compressed;
    {

        test::TestInput input = test::decompress_input(compressed);
        test::TestOutput output = test::decompress_output();

        c.decompress(input, output);

        compressed=output.result();
    }
    ASSERT_EQ(compression_string, compressed);
}




TEST(lfs2, no_strat){


    run_comp("abaabacabdab$");


}


