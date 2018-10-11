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
    auto c = Algorithm::instance<tdc::lfs::LFSCompressor<comp_strat, coding_strat >  > ();

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


TEST(lfs, bst_strat){
    typedef tdc::lfs::BSTStrategy esa_strat;

    run_comp<esa_strat >("");
    run_comp<esa_strat >("a");
    run_comp<esa_strat >("foobar");

    run_comp<esa_strat >("ab");
    run_comp<esa_strat >("abcd$");

    run_comp<esa_strat >("abab");

    run_comp<esa_strat >("abaaabbababb$");

    run_comp<esa_strat >("ccaabbaabbcca$");

}



TEST(lfs, sim_st_strat){

    typedef tdc::lfs::SimSTStrategy esa_strat;

    run_comp<esa_strat >("");
    run_comp<esa_strat >("a");
    run_comp<esa_strat >("foobar");

    run_comp<esa_strat >("ab");
    run_comp<esa_strat >("abcd$");

    run_comp<esa_strat >("abab");

    run_comp<esa_strat >("abaaabbababb$");

    run_comp<esa_strat >("ccaabbaabbcca$");

   // run_comp_file<tdc::lfs::SimSTStrategy<> >("english.1MB");
}

TEST(lfs, esa_strat){

    typedef tdc::lfs::ESAStrategy<> esa_strat;

    run_comp<esa_strat >("");
    run_comp<esa_strat >("a");
    run_comp<esa_strat >("foobar");

    run_comp<esa_strat >("ab");
    run_comp<esa_strat >("abcd$");

    run_comp<esa_strat >("abab");

    run_comp<esa_strat >("abaaabbababb$");

    run_comp<esa_strat >("ccaabbaabbcca$");


}

//TEST(lfs, esa_strat2){


//    run_comp<tdc::lfs::ESAStrategy<> >("abaaabbababb$");

//    run_comp<tdc::lfs::ESAStrategy<> >("ccaabbaabbcca$");


//}


