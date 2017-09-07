#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/rank/rank_64bit.hpp>

using namespace tdc;

TEST(rank_select, order) {
    // sanity check - test bit order in bit vector
    BitVector bv(64);
    bv[0] = 1;
    bv[8] = 1;
    ASSERT_EQ(257, bv.data()[0])
        << "rank/select requires LITTLE ENDIAN and LSBF order!";
}

TEST(rank, rank1_8bit) {
    // full vectors
    uint8_t v0   = 0b0000'0000; ASSERT_EQ(0, rank1(v0));
    uint8_t v2   = 0b0000'0011; ASSERT_EQ(2, rank1(v2));
    uint8_t v60  = 0b0011'1100; ASSERT_EQ(4, rank1(v60));
    uint8_t v255 = 0b1111'1111; ASSERT_EQ(8, rank1(v255));

    // interval [0,m]
    ASSERT_EQ(1, rank1(v2, 0));
    ASSERT_EQ(2, rank1(v2, 1));
    ASSERT_EQ(2, rank1(v2, 7));

    ASSERT_EQ(0, rank1(v60, 1));
    ASSERT_EQ(2, rank1(v60, 3));
    ASSERT_EQ(4, rank1(v60, 5));
    ASSERT_EQ(4, rank1(v60, 7));

    for(uint8_t i = 0; i < 8; i++) ASSERT_EQ(i+1, rank1(v255, i));

    // interval [l,m]
    ASSERT_EQ(1, rank1(v2, 0, 0));
    ASSERT_EQ(2, rank1(v2, 0, 1));
    ASSERT_EQ(2, rank1(v2, 0, 7));
    ASSERT_EQ(0, rank1(v2, 2, 7));

    ASSERT_EQ(0, rank1(v60, 0, 1));
    ASSERT_EQ(2, rank1(v60, 0, 3));
    ASSERT_EQ(4, rank1(v60, 2, 5));
    ASSERT_EQ(2, rank1(v60, 4, 7));
    ASSERT_EQ(0, rank1(v60, 6, 7));
    ASSERT_EQ(4, rank1(v60, 0, 7));

    for(uint8_t i = 0; i < 8; i++) ASSERT_EQ(1, rank1(v255, i, i));
}

TEST(rank, rank0_8bit) {
    // full vectors
    uint8_t v0   = 0b0000'0000; ASSERT_EQ(8, rank0(v0));
    uint8_t v2   = 0b0000'0011; ASSERT_EQ(6, rank0(v2));
    uint8_t v60  = 0b0011'1100; ASSERT_EQ(4, rank0(v60));
    uint8_t v255 = 0b1111'1111; ASSERT_EQ(0, rank0(v255));

    // interval [0,m]

    ASSERT_EQ(0, rank0(v2, 0));
    ASSERT_EQ(0, rank0(v2, 1));
    ASSERT_EQ(6, rank0(v2, 7));

    ASSERT_EQ(2, rank0(v60, 1));
    ASSERT_EQ(2, rank0(v60, 3));
    ASSERT_EQ(2, rank0(v60, 5));
    ASSERT_EQ(4, rank0(v60, 7));

    for(uint8_t i = 0; i < 8; i++) ASSERT_EQ(i+1, rank0(v0, i));

    // interval [l,m]
    ASSERT_EQ(0, rank0(v2, 0, 0));
    ASSERT_EQ(0, rank0(v2, 0, 1));
    ASSERT_EQ(6, rank0(v2, 0, 7));
    ASSERT_EQ(6, rank0(v2, 2, 7));

    ASSERT_EQ(2, rank0(v60, 0, 1));
    ASSERT_EQ(2, rank0(v60, 0, 3));
    ASSERT_EQ(0, rank0(v60, 2, 5));
    ASSERT_EQ(2, rank0(v60, 4, 7));
    ASSERT_EQ(2, rank0(v60, 6, 7));
    ASSERT_EQ(4, rank0(v60, 0, 7));

    for(uint8_t i = 0; i < 8; i++) ASSERT_EQ(1, rank0(v0, i, i));
}

TEST(rank, rank1_16_32_64bit) {
    // full vectors
    uint16_t v16 = 0x0101;
    uint32_t v32 = 0x01010101UL;
    uint64_t v64 = 0x0101010101010101ULL;

    ASSERT_EQ(2, rank1(v16));
    ASSERT_EQ(4, rank1(v32));
    ASSERT_EQ(8, rank1(v64));

    // interval [0, m]
    for(uint8_t i = 1; i <= 2; i++) ASSERT_EQ(i, rank1(v16, 8*i-1));
    for(uint8_t i = 1; i <= 4; i++) ASSERT_EQ(i, rank1(v32, 8*i-1));
    for(uint8_t i = 1; i <= 8; i++) ASSERT_EQ(i, rank1(v64, 8*i-1));

    // interval [l, m]
    for(uint8_t i = 1; i <= 2; i++) ASSERT_EQ(1, rank1(v16, 8*(i-1), 8*i-1));
    for(uint8_t i = 1; i <= 4; i++) ASSERT_EQ(1, rank1(v32, 8*(i-1), 8*i-1));
    for(uint8_t i = 1; i <= 8; i++) ASSERT_EQ(1, rank1(v64, 8*(i-1), 8*i-1));
}

TEST(rank, rank0_16_32_64bit) {
    // full vectors
    uint16_t v16 = 0x0101;
    uint32_t v32 = 0x01010101UL;
    uint64_t v64 = 0x0101010101010101ULL;

    ASSERT_EQ(14, rank0(v16));
    ASSERT_EQ(28, rank0(v32));
    ASSERT_EQ(56, rank0(v64));

    // interval [0, m]
    for(uint8_t i = 1; i <= 2; i++) ASSERT_EQ(7*i, rank0(v16, 8*i-1));
    for(uint8_t i = 1; i <= 4; i++) ASSERT_EQ(7*i, rank0(v32, 8*i-1));
    for(uint8_t i = 1; i <= 8; i++) ASSERT_EQ(7*i, rank0(v64, 8*i-1));

    // interval [l, m]
    for(uint8_t i = 1; i <= 2; i++) ASSERT_EQ(7, rank0(v16, 8*(i-1), 8*i-1));
    for(uint8_t i = 1; i <= 4; i++) ASSERT_EQ(7, rank0(v32, 8*(i-1), 8*i-1));
    for(uint8_t i = 1; i <= 8; i++) ASSERT_EQ(7, rank0(v64, 8*(i-1), 8*i-1));
}
