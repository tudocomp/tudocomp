#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/rank_64bit.hpp>
#include <tudocomp/ds/select_64bit.hpp>
#include <tudocomp/ds/Rank.hpp>

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

TEST(rank, uint_16_32_64) {
    // full vectors
    uint16_t v16 = 0x0101;
    uint32_t v32 = 0x01010101UL;
    uint64_t v64 = 0x0101010101010101ULL;

    ASSERT_EQ(2, rank1(v16));
    ASSERT_EQ(4, rank1(v32));
    ASSERT_EQ(8, rank1(v64));

    // interval [0, m]
    for(uint8_t i = 1; i <= 2; i++) {
        ASSERT_EQ(i, rank1(v16, 8*i-1));
        ASSERT_EQ(i, rank0(uint16_t(~v16), 8*i-1));
    }
    for(uint8_t i = 1; i <= 4; i++) {
        ASSERT_EQ(i, rank1(v32, 8*i-1));
        ASSERT_EQ(i, rank0(uint32_t(~v32), 8*i-1));
    }
    for(uint8_t i = 1; i <= 8; i++) {
        ASSERT_EQ(i, rank1(v64, 8*i-1));
        ASSERT_EQ(i, rank0(uint64_t(~v64), 8*i-1));
    }

    // interval [l, m]
    for(uint8_t i = 1; i <= 2; i++) {
        ASSERT_EQ(1, rank1(v16, 8*(i-1), 8*i-1));
        ASSERT_EQ(1, rank0(uint16_t(~v16), 8*(i-1), 8*i-1));
    }
    for(uint8_t i = 1; i <= 4; i++) {
        ASSERT_EQ(1, rank1(v32, 8*(i-1), 8*i-1));
        ASSERT_EQ(1, rank0(uint32_t(~v32), 8*(i-1), 8*i-1));
    }
    for(uint8_t i = 1; i <= 8; i++) {
        ASSERT_EQ(1, rank1(v64, 8*(i-1), 8*i-1));
        ASSERT_EQ(1, rank0(uint64_t(~v64), 8*(i-1), 8*i-1));
    }
}

TEST(rank, rank_bv) {
    const size_t N = 16384; // amount of bits
    const size_t K = 4;     // set every K-th bit

    BitVector bv(N);

    // set every K-th bit
    for(size_t i = 0; i < N; i += K) bv[i] = 1;

    // construct rank data structure
    Rank rank(bv, N/2);

    // rank1
    ASSERT_EQ(N/K, rank(N-1));
    for(size_t i = 1; i <= N/K; i++) ASSERT_EQ(i, rank(K*i-1));
    for(size_t i = 1; i <= N/K; i++) ASSERT_EQ(1, rank(K*(i-1), K*i-1));

    // rank0
    ASSERT_EQ(N-N/K, rank.rank0(N-1));
    for(size_t i = 1; i <= N/K; i++) ASSERT_EQ((K-1)*i, rank.rank0(K*i-1));
    for(size_t i = 1; i <= N/K; i++) ASSERT_EQ(K-1, rank.rank0(K*(i-1), K*i-1));
}

TEST(select, basic) {
    uint64_t v0 = 0ULL;
    uint64_t v1 = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t v64 = 0x0101010101010101ULL;

    for(size_t i = 1; i <= 64; i++) {
        ASSERT_EQ(i-1, select1(v1, i));
        ASSERT_EQ(SELECT_FAIL, select1(v0, i));
        ASSERT_EQ(i-1, select0(v0, i));
        ASSERT_EQ(SELECT_FAIL, select0(v1, i));
    }

    for(size_t i = 1; i <= 8; i++) {
        ASSERT_EQ(8*(i-1), select1(v64, i));
        ASSERT_EQ(8*(i-1), select0(uint64_t(~v64), i));
    }
}

TEST(rank_select, inverse_property) {
    uint64_t v64 = 0x0101010101010101ULL;

    for(size_t i = 1; i <= 8; i++) {
        ASSERT_EQ(i, rank1(v64,            select1(v64, i)));
        ASSERT_EQ(i, rank0(uint64_t(~v64), select0(uint64_t(~v64), i)));
    }

    for(size_t i = 0; i < 64; i++) {
        ASSERT_GE(i, select1(v64,            rank1(v64, i)));
        ASSERT_GE(i, select0(uint64_t(~v64), rank0(uint64_t(~v64), i)));
    }
}
