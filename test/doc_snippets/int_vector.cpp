/**
 *
 * This file contains code snippets from the documentation as a reference.
 *
 * Please do not change this file unless you change the corresponding snippets
 * in the documentation as well!
 *
**/

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <numeric>

#include <tudocomp/def.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/ds/IntVector.hpp>

using namespace tdc;

TEST(doc_int_vector, iv_static) {
    // reserve a vector of 32 4-bit integers (initialized as zero)
    IntVector<uint_t<4>> iv4(32);

    // fill it with increasing values (0, 1, 2, ...)
    std::iota(iv4.begin(), iv4.end(), 0);

    // print size in bits
    ASSERT_EQ(128, iv4.bit_size());

    // demonstrate overflow
    ASSERT_EQ(iv4[0], iv4[16]);

    // reserve an additional bit vector with 32 entries
    BitVector bv(32);

    // mark all multiples of 3
    for(len_t i = 0; i < 32; i++) bv[i] = ((iv4[i] % 3) == 0);

    ASSERT_EQ(1UL, bv[0]);
    ASSERT_EQ(0UL, bv[7]);
    ASSERT_EQ(1UL, bv[15]);
}

TEST(doc_int_vector, iv_dynamic) {
    // reserve a vector for 20 integer values (initialized as zero)
    // default to a width of 32 bits per value
    DynamicIntVector fib(20, 0, 32);

    ASSERT_EQ(640, fib.bit_size());

    // fill it with the Fibonacci sequence
    fib[0] = 0;
    fib[1] = 1;
    for(len_t i = 2; i < fib.size(); i++)
      fib[i] = fib[i - 2] + fib[i - 1];

    // find the amount of bits required to store the last (and largest) value
    auto max_bits = bits_for(fib.back());

    // bit-compress the vector to the amount of bits required
    fib.width(max_bits);
    fib.shrink_to_fit();

    ASSERT_EQ(260, fib.bit_size());
}

