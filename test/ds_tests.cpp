#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tudocomp/io.h>
#include <tudocomp/util/Generators.hpp>
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
    "0",
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

size_t lce(const std::string& text, size_t a, size_t b) {
	DCHECK_NE(a,b);
	size_t i = 0;
	while(text[a+i] == text[b+i]) { ++i; }
	return i;
}


// === THE ACTUAL TESTS ===
template<class textds_t>
void test_TestSA(const std::string& str) {
    DLOG(INFO) << "str = \"" << str << "\"" << " size: " << str.length();

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
		ASSERT_LT(str.substr(sa[i-1]), str.substr(sa[i])); //TODO: use basic_string_view to speed up!
    }

    auto& phi = t.require_phi();
    ASSERT_EQ(phi.size(), sa.size()); //length
    ASSERT_PERMUTATION(phi, phi.size()); //permutation

	for(size_t i = 0; i < sa.size(); ++i) {
		ASSERT_EQ(phi[sa[(i+1) % sa.size()]], sa[i]);
	}
	

    auto& isa = t.require_isa();
    ASSERT_EQ(isa.size(), sa.size()); 
	for(size_t i = 0; i < sa.size(); ++i) {
		ASSERT_EQ(isa[sa[i]], i);
	}

    auto& lcp = t.require_lcp();
    ASSERT_EQ(lcp.size(), sa.size()); //length

	for(size_t i = 1; i < lcp.size(); ++i) {
		ASSERT_EQ(lcp[i], lce(str, sa[i], sa[i-1]));
	}

	{
	size_t plcp[lcp.size()];
	for(size_t i = 0; i < lcp.size(); ++i)
		plcp[i] = lcp[isa[i]];
	for(size_t i = 1; i < lcp.size(); ++i)
		ASSERT_GE(plcp[i]+1, plcp[i-1]);
	}
	// std::sort(plcp);
	// const size_t maxlcp = *std::max_element(plcp,plcp+lcp.size());
	// size_t oldbound = std::lower_bound(plcp,plcp+lcp.size(),1);
	// size_t oldrange= oldbound;
	// for(size_t i = 2; i < maxlcp; ++i) {
	// 	size_t bound = std::lower_bound(plcp,plcp+lcp.size(),i);
	// 	ASSERT_GE(
    //
	// }

}

TEST(ds, TestSA) {
    run_tests<StringArrayDispenser>(&test_TestSA<TextDS<>>);
	for(size_t i = 0; i < 4; ++i) {
		std::string s = fibonacci_word(1<<i);
		test_TestSA<TextDS<>>(s);
	}
	for(size_t i = 0; i < 11; ++i) {
		for(size_t j = 0; j < 2+50/(i+1); ++j) {
			std::string s = random_uniform(1<<i,Ranges::numbers,j);
			test_TestSA<TextDS<>>(s);
		}
	}
}
