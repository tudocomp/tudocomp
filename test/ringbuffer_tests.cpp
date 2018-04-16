#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/ds/RingBuffer.hpp>

using namespace tdc;

constexpr size_t N = 8;

TEST(ringbuffer, init) {
    RingBuffer<size_t> r(N);

    ASSERT_TRUE(r.empty());
    ASSERT_FALSE(r.full());
    ASSERT_EQ(0, r.size());
    ASSERT_EQ(N, r.capacity());
    ASSERT_EQ(r.begin(), r.end());
    ASSERT_EQ(r.cbegin(), r.cend());
}

TEST(ringbuffer, push) {
    RingBuffer<size_t> r(N);

    for(size_t i = 0; i < N; i++) {
        ASSERT_FALSE(r.full());
        r.push_back(i);
        ASSERT_EQ(i+1, r.size());
        ASSERT_FALSE(r.empty());
    }
    ASSERT_TRUE(r.full());
}

TEST(ringbuffer, pop) {
    RingBuffer<size_t> r(N);
    for(size_t i = 0; i < N; i++) r.push_back(i);
    ASSERT_TRUE(r.full());
    for(size_t i = 0; i < N; i++) ASSERT_EQ(i, r.pop_front());
    ASSERT_TRUE(r.empty());
}

TEST(ringbuffer, push_pop) {
    RingBuffer<size_t> r(N);
    for(size_t i = 0; i < N; i++) {
        r.push_back(i);
        ASSERT_EQ(i, r.pop_front());
        ASSERT_TRUE(r.empty());
    }
}

TEST(ringbuffer, iterator) {
    RingBuffer<size_t> r(N);
    for(size_t i = 0; i < N; i++) r.push_back(i);
    
    size_t k = 0;
    for(auto x : r) ASSERT_EQ(k++, x);
}

TEST(ringbuffer, iterator_offset) {
    RingBuffer<size_t> r(N);
    for(size_t i = 0; i < N; i++) r.push_back(i);
    for(size_t k = 0; k < N; k++) ASSERT_EQ(k, *(r.begin() + k));
    ASSERT_EQ(r.end(), r.begin() + r.size());
}

TEST(ringbuffer, circular) {
    RingBuffer<size_t> r(N);
    for(size_t i = 0; i < 2 * N; i++) r.push_back(i);
    
    size_t k = N;
    for(auto x : r) ASSERT_EQ(k++, x);
}

TEST(ringbuffer, underfull) {
    RingBuffer<size_t> r(N);
    for(size_t i = 0; i < N / 2; i++) r.push_back(i);
    
    size_t k = 0;
    for(auto x : r) ASSERT_EQ(k++, x);
}

