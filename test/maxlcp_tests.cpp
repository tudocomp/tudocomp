#include <glog/logging.h>
#include <gtest/gtest.h>

#include <random>

#include <tudocomp/ds/ArrayMaxHeap.hpp>
#include <tudocomp/compressors/lcpcomp/MaxLCPSuffixList.hpp>
#include "test/util.hpp"

using namespace tdc;

using vec = std::vector<size_t>;

std::default_random_engine rglobal;
std::uniform_int_distribution<size_t> gen_seed(0, SIZE_MAX);
std::uniform_int_distribution<size_t> gen_dice(1, 6);

// small test
constexpr size_t MIN_POSSIBLE_LCP = 2;
constexpr size_t MAX_POSSIBLE_LCP = 2048;
constexpr size_t NUM_TESTS = 100;
std::uniform_int_distribution<size_t> gen_size(1000, 2500);

// LARGE test
//constexpr size_t MIN_POSSIBLE_LCP = 2;
//constexpr size_t MAX_POSSIBLE_LCP = 65536;
//constexpr size_t NUM_TESTS = 10;
//std::uniform_int_distribution<size_t> gen_size(200000, 250000);

void generate_lcp(const size_t n, const size_t seed, vec& out_lcp, size_t& out_max) {
    // generate dataset
    std::default_random_engine rnd(seed);
    std::uniform_int_distribution<size_t> dist(MIN_POSSIBLE_LCP, MAX_POSSIBLE_LCP);

    out_lcp = vec(n);
    out_lcp[0] = 0; // invariant
    for(size_t i = 1; i < out_lcp.size(); i++) out_lcp[i] = dist(rnd);
    out_max = *std::max_element(out_lcp.begin(), out_lcp.end());
}

template<typename ds_t>
void test_remove_only(const size_t n, const size_t seed, ds_t& ds, vec& lcp) {
    // test item order, removing one by one
    VLOG(2) << "  Testing ...";
    size_t last = ds.get_max();
    ds.remove(last);

    while(ds.size()) {
        size_t m = ds.get_max();
        ASSERT_GE(lcp[last], lcp[m]) << "n = " << n << ", seed = " << seed;

        ds.remove(m);
        last = m;
    }
}

template<typename ds_t>
void test_dec_key(const size_t n, const size_t seed, ds_t& ds, vec& lcp) {
    // test item order, call decrease key for random items by random amounts
    VLOG(2) << "  Testing ...";
    size_t last = ds.get_max();
    ds.remove(last);

    std::default_random_engine rnd(seed);
    while(ds.size()) {
        size_t m = ds.get_max();
        ASSERT_GE(lcp[last], lcp[m]) << "n = " << n << ", seed = " << seed;

        if(lcp[m] > MIN_POSSIBLE_LCP && gen_dice(rnd) <= 3) {
            // decrease key
            size_t amt = std::min(gen_dice(rnd), lcp[m] - MIN_POSSIBLE_LCP);
            ds.decrease_key(m, lcp[m] - amt);
            last = ds.get_max();
        } else {
            // remove
            ds.remove(m);
            last = m;
        }
    }
}

void test_heap_remove_only(const size_t n, const size_t seed) {
    // generate
    VLOG(2) << "  Generating ...";
    vec lcp; size_t max_lcp;
    generate_lcp(n, seed, lcp, max_lcp);

    VLOG(2) << "  Inserting ...";
    ArrayMaxHeap<vec> ds(lcp, n, n);
    for(size_t i = 1; i < n; i++) ds.insert(i);

    // test size
    ASSERT_EQ(n-1, ds.size());

    // test
    test_remove_only(n, seed, ds, lcp);
}

void test_heap_dec_key(const size_t n, const size_t seed) {
    // generate
    VLOG(2) << "  Generating ...";
    vec lcp; size_t max_lcp;
    generate_lcp(n, seed, lcp, max_lcp);

    VLOG(2) << "  Inserting ...";
    ArrayMaxHeap<vec> ds(lcp, n, n);
    for(size_t i = 1; i < n; i++) ds.insert(i);

    // test size
    ASSERT_EQ(n-1, ds.size());

    // test
    test_dec_key(n, seed, ds, lcp);
}

void test_list_remove_only(const size_t n, const size_t seed) {
    // generate
    VLOG(2) << "  Generating ...";
    vec lcp; size_t max_lcp;
    generate_lcp(n, seed, lcp, max_lcp);

    VLOG(2) << "  Inserting ...";
    lcpcomp::MaxLCPSuffixList<vec> ds(lcp, MIN_POSSIBLE_LCP, max_lcp);

    // test size
    ASSERT_EQ(n-1, ds.size());

    // test
    test_remove_only(n, seed, ds, lcp);
}

void test_list_dec_key(const size_t n, const size_t seed) {
    // generate
    VLOG(2) << "  Generating ...";
    vec lcp; size_t max_lcp;
    generate_lcp(n, seed, lcp, max_lcp);

    VLOG(2) << "  Inserting ...";
    lcpcomp::MaxLCPSuffixList<vec> ds(lcp, MIN_POSSIBLE_LCP, max_lcp);

    // test size
    ASSERT_EQ(n-1, ds.size());

    // test
    test_dec_key(n, seed, ds, lcp);
}

template<typename testfunc_t>
void test_run(testfunc_t f) {
    for(size_t i = 0; i < NUM_TESTS; i++) {
        size_t n = gen_size(rglobal);
        size_t seed = gen_seed(rglobal);
        VLOG(1) << "n = " << n << ", seed = " << seed;
        VLOG(2) << "(run " << (i+1) << " of " << NUM_TESTS << ")"; // for tests with LARGE sets
        f(n, seed);
    }
}

TEST(MaxLCP, heap_remove_only) {
    test_run(test_heap_remove_only);
}

TEST(MaxLCP, list_remove_only) {
    test_run(test_list_remove_only);
}

TEST(MaxLCP, heap_dec_key) {
    test_run(test_heap_dec_key);
}

TEST(MaxLCP, list_dec_key) {
    test_run(test_list_dec_key);
}

