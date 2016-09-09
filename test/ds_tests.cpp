#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tudocomp/io.h>
#include <tudocomp/ds/TextDS.hpp>

using namespace tudocomp;

typedef void (*string_test_func_t)(const std::string&);

//Check that p is a permutation of [0..n-1]
template<class T>
void ASSERT_PERMUTATION(const T& p, size_t n) {
    for(size_t i = 0; i < n; ++i)
    for(size_t j = 0; j < n; ++j)
    {
        if(i == j) continue;
        ASSERT_NE(p[i],p[j]) << "at positions " << i << " and " << j;
        ASSERT_LT(p[i],n);
    }
}

// Simple string array dispenser
class StringArrayDispenser {
private:
    static const size_t NUM_TEST_STRINGS;
    static const std::string TEST_STRINGS[];

    size_t m_next;

public:
    StringArrayDispenser() : m_next(0) {};

    bool has_next() {
        return m_next < NUM_TEST_STRINGS;
    }

    const std::string& next() {
        return TEST_STRINGS[m_next++];
    }
};

const size_t StringArrayDispenser::NUM_TEST_STRINGS = 4;
const std::string StringArrayDispenser::TEST_STRINGS[] = {
    "",
    "aaaaaaa",
    "aabaaababaaabbaabababaab",
    "fjgwehfwbz43bngkwrp23fa"
};

//TODO: Markov chain generator

template<class generator_t>
void run_tests(string_test_func_t test) {
    generator_t generator;
    while(generator.has_next()) {
        test(generator.next());
    }
}

// === THE ACTUAL TESTS ===
template<class textds_t>
void test_TestSA(const std::string& str) {
    DLOG(INFO) << "str = \"" << str << "\"";

    size_t n = str.length();
    Input input(str);
    textds_t t(input.as_view());
    auto& sa = t.require_sa();

    ASSERT_EQ(sa.size(), n+1); //length
    ASSERT_PERMUTATION(sa, n+1); //permutation
    ASSERT_EQ(sa[0], n); //first element is $

    //check lexicographic order
    for(size_t i = 1; i < n+1; i++) {
        ASSERT_GE(t[sa[i]], t[sa[i-1]]);
    }
}

TEST(ds, TestSA) {
    run_tests<StringArrayDispenser>(&test_TestSA<TextDS<>>);
}
