//Test the longest first substitution

#include <tudocomp/compressors/lfs/ESALFSCompressor.hpp>
#include <tudocomp/compressors/lfs/STLFSCompressor.hpp>

#include <tudocomp/compressors/lfs/LFSCompressor.hpp>
#include <tudocomp/compressors/lfs/STStrategy.hpp>

#include <tudocomp/compressors/lfs/ESAStrategy.hpp>

#include <tudocomp/compressors/lfs/SimSTStrategy.hpp>

#include <tudocomp/compressors/lfs/EncodeStrategy.hpp>

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




template<class comp_strat>
void run_comp(std::string compression_string) {
    auto c = create_algo<tdc::lfs::LFSCompressor<comp_strat> >();

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


/*
template<class comp_strat>
void run_comp_file(std::string file) {
    auto c = create_algo<tdc::lfs::LFSCompressor<comp_strat> >();

    //std::string compressed;
    // compress
    {
        tdc::Input dummy_input = Input::Path{file};
        tdc::Output output = Output(file+".tdc", true);

        c.compress(dummy_input, output);
       // compressed=output.result();

    }
    // decompress
    {

        tdc::Input input = Input::Path{file+".tdc"};
        tdc::Output output = Output(file+".dc", true);;

        c.decompress(input, output);

       // compressed=output.result();
    }
    //ASSERT_EQ(compression_string, compressed);
}


*/

TEST(lfs, st_strat){


    run_comp<tdc::lfs::STStrategy<> >("abaaabbababb$");

    run_comp<tdc::lfs::STStrategy<> >("ccaabbaabbcca$");

}

TEST(lfs, sim_st_strat){


    run_comp<tdc::lfs::SimSTStrategy<> >("abaaabbababb$");

    run_comp<tdc::lfs::SimSTStrategy<> >("ccaabbaabbcca$");

   // run_comp_file<tdc::lfs::SimSTStrategy<> >("english.1MB");
}

TEST(lfs, esa_strat){


    run_comp<tdc::lfs::ESAStrategy<> >("abaaabbababb$");

    run_comp<tdc::lfs::ESAStrategy<> >("ccaabbaabbcca$");


}


