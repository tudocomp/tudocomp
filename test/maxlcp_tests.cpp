#include <glog/logging.h>
#include <gtest/gtest.h>

#include <random>

#include <tudocomp/compressors/esacomp/MaxLCPHeap.hpp>
#include <tudocomp/compressors/esacomp/MaxLCPSuffixList.hpp>
#include "test/util.hpp"

using namespace tdc;

using vec = std::vector<size_t>;

constexpr size_t MIN_POSSIBLE_LCP = 2ULL;
constexpr size_t MAX_POSSIBLE_LCP = 65536ULL;

template<typename ds_t>
void test_max_lcp_order(size_t n) {
    // generate dataset
    std::default_random_engine rnd;
    std::uniform_int_distribution<size_t> dist(MIN_POSSIBLE_LCP, MAX_POSSIBLE_LCP);

    vec lcp(n);
    lcp[0] = 0; // invariant
    for(size_t i = 1; i < lcp.size(); i++) lcp[i] = dist(rnd);
    size_t max = *std::max_element(lcp.begin(), lcp.end());
    
    // construct max lcp data structure
    ds_t ds(lcp, MIN_POSSIBLE_LCP, max);

    // test item order
    ASSERT_EQ(n-1, ds.size());

    size_t last = ds.get_max();
    ds.remove(last);

    while(ds.size()) {
        size_t m = ds.get_max();
        ASSERT_GE(lcp[last], lcp[m]);

        last = m;
        ds.remove(last);
    }
}

TEST(MaxLCP, heap) {
    test_max_lcp_order<esacomp::MaxLCPHeap<vec>>(100000);
}

TEST(MaxLCP, list) {
    test_max_lcp_order<esacomp::MaxLCPSuffixList<vec>>(100000);
}

