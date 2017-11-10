/**
 *
 * This file contains code snippets from the documentation as a reference.
 *
 * Please do not change this file unless you change the corresponding snippets
 * in the documentation as well!
 *
**/

#include <numeric>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <numeric>

#include <tudocomp/def.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/ds/Rank.hpp>
#include <tudocomp/ds/Select.hpp>

using namespace tdc;

TEST(doc_bit_vector, bv_rank_select) {
    // Construct a bit vector where every second bit is set
    BitVector bv(128);
    for(len_t i = 1; i < 128; i += 2) bv[i] = 1;

    // Construct Rank and Select1 data structures
    Rank    rank(bv);
    Select1 select1(bv);

    // Query the amount of 1-bits in the whole bit vector
    ASSERT_EQ(64, rank.rank1(127));

    // Query the amount of 0-bits in the whole bit vector
    ASSERT_EQ(64, rank.rank0(127));

    // Query the amount of 1-bits in the second half of the bit vector
    ASSERT_EQ(32, rank.rank0(64, 127));

    // Find the position of the first 1-bit
    ASSERT_EQ(1,  select1(1));

    // Find the position of the 32nd 1-bit
    ASSERT_EQ(63, select1(32));

    // Find the position of the 1000th 1-bit (which does not exist)
    ASSERT_EQ(bv.size(), select1(1000));

    // rank1(select1(i)) = i holds
    for(len_t i = 1; i <= 64; i++) {
        ASSERT_EQ(i, rank(select1(i)));
    }
}


