//Test the longest first substitution

#include <tudocomp/compressors/lfs/LFSCompressor.hpp>
#include <tudocomp/compressors/lfs/STStrategy.hpp>

#include <tudocomp/compressors/lfs/BSTStrategy.hpp>

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
    // , tdc::lfs::EncodeStrategy< tdc::ASCIICoder, tdc::ASCIICoder
    typedef tdc::lfs::EncodeStrategy< tdc::ASCIICoder, tdc::ASCIICoder >  coding_strat;
    auto c = create_algo<tdc::lfs::LFSCompressor<comp_strat, coding_strat >  > ();

    std::string compressed;
    // compress
    {
        test::TestInput dummy_input (compression_string, true);
        test::TestOutput output = test::compress_output();

        c.compress(dummy_input, output);
        compressed=output.result();
        DLOG(INFO)<<"compressed: "<< compressed;

    }
    // decompress
    {

        test::TestInput input (compressed, false);
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
        tdc::Input dummy_input = Input(io::Path{file});
        tdc::Output output = Output(io::Path{file+".tdc"}, true);

        c.compress(dummy_input, output);
       // compressed=output.result();

    }
    // decompress
    {

        tdc::Input input = Input(io::Path{file+".tdc"});
        tdc::Output output = Output(io::Path{file+".dc"}, true);;

        c.decompress(input, output);

       // compressed=output.result();
    }
    //ASSERT_EQ(compression_string, compressed);
}*/


TEST(lfs, bst_strat){


    run_comp<tdc::lfs::BSTStrategy >("abaaabbababb$");

    run_comp<tdc::lfs::BSTStrategy >("ccaabbaabbcca$");

}

/*

TEST(lfs, sim_st_strat){


    run_comp<tdc::lfs::SimSTStrategy >("abaaabbababb$");

    run_comp<tdc::lfs::SimSTStrategy >("ccaabbaabbcca$");

   // run_comp_file<tdc::lfs::SimSTStrategy<> >("english.1MB");
}

TEST(lfs, esa_strat){


    run_comp<tdc::lfs::ESAStrategy<> >("abaaabbababb$");

    run_comp<tdc::lfs::ESAStrategy<> >("ccaabbaabbcca$");


}

TEST(lfs, esa_strat){


    run_comp<tdc::lfs::ESAStrategy<> >("abaaabbababb$");

    run_comp<tdc::lfs::ESAStrategy<> >("ccaabbaabbcca$");


}*/


