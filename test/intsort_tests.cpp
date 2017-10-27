#include <gtest/gtest.h>

#define ENABLE_OPENMP

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <random>

#include <tudocomp/util/IntSort.hpp>

template <typename T, typename Key, typename KeyExtract, typename Less>
void compare_sort(std::vector<T>& input, KeyExtract extract, Key max_key, Less less) {
    std::vector<T> vec_ref(input);
    std::vector<T> input2(input);

    intsort(input.begin(), input.end(), extract, max_key);
    intsort(input2, extract, max_key);
    std::sort(vec_ref.begin(), vec_ref.end(), less);

    for(size_t i = 0; i < input.size(); ++i)
        ASSERT_EQ(input[i], vec_ref[i]) << "i=" << i;

    for(size_t i = 0; i < input.size(); ++i)
        ASSERT_EQ(input2[i], vec_ref[i]) << "i=" << i;
};


template <typename T>
void random_int_test(size_t n, T max_key, int seed) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<T> dist(0, max_key);

    std::vector<T> vec_test;
    vec_test.reserve(n);
    for(size_t i=0; i != n; ++i)
        vec_test.push_back(dist(gen));

    compare_sort(vec_test, [] (T x) {return x;}, max_key, [] (T a, T b) {return a < b;});
}

template <typename T>
void random_test_suite() {
    for(unsigned int i = 1; i != 10; ++i) {
        const size_t len = 123 + pow(i, 6.1);
        const T max_key = (0x12345678 * i) ^ (0x31415923 * i);
        random_int_test(len, max_key, i);
    }
}


TEST(IntSort, basic_test) {
    using T = uint32_t;
    std::vector<T> input = {0, 1, 2, 3, 0x1000, 0x1001, 0x1002, 0x1003, 0x1007, 0x1006, 7, 6, 5, 4, 0x1005, 0x1004};
    compare_sort(input, [] (T x) {return x;}, 0x1008, [] (T a, T b) {return a < b;});
}

TEST(IntSort, random_uint8) {random_test_suite<uint8_t>();}
TEST(IntSort, random_uint16) {random_test_suite<uint16_t>();}
TEST(IntSort, random_uint32) {random_test_suite<uint32_t>();}
TEST(IntSort, random_uint64) {random_test_suite<uint64_t>();}
