#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tudocomp/io.hpp>
#include <tudocomp/util/Generators.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include "test_util.hpp"

using namespace tdc;

size_t longest_common_extension(const std::string& text, size_t a, size_t b) {
	DCHECK_NE(a,b);
	size_t i = 0;
	while(text[a+i] == text[b+i]) { ++i; }
	return i;
}

template<class sa_t>
sdsl::int_vector<> create_lcp_naive(const std::string& text, const sa_t& sa) {
	sdsl::int_vector<> lcp(sa.size());
	lcp[0]=0;
	for(size_t i = 1; i < sa.size(); ++i) {
		lcp[i] = longest_common_extension(text, sa[i],sa[i-1]);
	}
	return lcp;
}

template<class lcp_t, class isa_t>
sdsl::int_vector<> create_plcp_naive(const lcp_t& lcp, const isa_t& isa) {
	sdsl::int_vector<> plcp(lcp.size());
	for(size_t i = 0; i < lcp.size(); ++i) {
		DCHECK_LT(isa[i], lcp.size());
		DCHECK_GE(isa[i], 0);
		plcp[i] = lcp[isa[i]];
	}
	for(size_t i = 1; i < lcp.size(); ++i) {
		DCHECK_GE(plcp[i]+1, plcp[i-1]);
	}
	return plcp;
}


// === THE ACTUAL TESTS ===
template<class textds_t>
void test_sa(const std::string& str, textds_t& t) {
    auto& sa = t.require_sa();

    ASSERT_EQ(sa.size(), str.length()+1); //length
    assert_permutation(sa, str.length()+1); //permutation
    ASSERT_EQ(sa[0], str.length()); //first element is $

    //lexicographic order
    for(size_t i = 1; i < t.size()+1; i++) {
        ASSERT_GE(t[sa[i]], t[sa[i-1]]);
		ASSERT_LT(str.substr(sa[i-1]), str.substr(sa[i]));
		ASSERT_LT(View(str,sa[i-1]), View(str,sa[i]));
    }
}

template<class textds_t>
void test_isa(const std::string& str, textds_t& t) {
    auto& isa = t.require_isa();
    auto& sa  = t.require_sa(); //request afterwards!

    ASSERT_EQ(isa.size(), sa.size()); //length

    //correctness
	for(size_t i = 0; i < sa.size(); ++i) {
		ASSERT_EQ(isa[sa[i]], i);
	}
}

template<class textds_t>
void test_phi(const std::string& str, textds_t& t) {
    auto& phi = t.require_phi();
    auto& sa  = t.require_sa(); //request afterwards!

    ASSERT_EQ(phi.size(), sa.size()); //length
    assert_permutation(phi, phi.size()); //permutation

    //correctness
	for(size_t i = 0; i < sa.size(); ++i) {
		ASSERT_EQ(phi[sa[(i+1) % sa.size()]], sa[i]);
	}
}

template<class textds_t>
void test_lcp(const std::string& str, textds_t& t) {
    auto& lcp = t.require_lcp();
    auto& sa  = t.require_sa(); //request afterwards!

    ASSERT_EQ(lcp.size(), sa.size()); //length

    //correctness
	for(size_t i = 1; i < lcp.size(); ++i) {
		ASSERT_EQ(lcp[i], longest_common_extension(str, sa[i], sa[i-1]));
	}
}

template<class textds_t>
void test_lcp_naive(const std::string& str, textds_t& t) {
    auto& sa = t.require_sa();
    auto& isa = t.require_isa();

    const sdsl::int_vector<> lcp_naive(create_lcp_naive(str,sa));
	const auto plcp = LCP::phi_algorithm(t);
	const auto plcp_naive = create_plcp_naive(lcp_naive, isa);
	//assert_eq_sequence(plcp, plcp_naive);

    auto& lcp = t.require_lcp();
	//assert_eq_sequence(lcp, lcp_naive);
    ASSERT_EQ(lcp.size(), sa.size()); //length
	for(size_t i = 1; i < lcp.size(); ++i) {
		ASSERT_EQ(lcp_naive[i], plcp_naive[sa[i]]) << "for i=" << i;
		ASSERT_EQ(lcp[i], plcp[sa[i]]) << "for i=" << i;
		ASSERT_EQ(lcp[i], plcp_naive[sa[i]]) << "for i=" << i;
	}
}

template<class textds_t>
void test_all_ds(const std::string& str, textds_t& t) {
    test_sa(str, t);
    test_isa(str, t);
    test_phi(str, t);
    test_lcp(str, t);
    test_lcp_naive(str, t);
}

template<class textds_t>
void test_ds(const std::string& str, void (*testfunc)(const std::string&, textds_t&)) {
    DLOG(INFO) << "str = \"" << str << "\"" << " size: " << str.length();

    Input input(str);
    textds_t t(input.as_view());
    testfunc(str, t);
}

#define TEST_COLLECTION(testfunc) \
    test_ds<TextDS<>>("", &testfunc); \
    test_ds<TextDS<>>("aaaaaaaaa", &testfunc); \
    test_ds<TextDS<>>("banana", &testfunc); \
    test_ds<TextDS<>>("abcdefgh#defgh_abcde", &testfunc); \

#define TEST_FIBONACCI(testfunc, n) \
	for(size_t i = 0; i < n; ++i) { \
		std::string s = fibonacci_word(1); \
        test_ds<TextDS<>>(s, &testfunc); \
	}

#define TEST_THUEMORSE(testfunc, n) \
	for(size_t i = 0; i < n; ++i) { \
		std::string s = thue_morse_word(1); \
        test_ds<TextDS<>>(s, &testfunc); \
	}
#define TEST_RUNRICH(testfunc, n) \
	for(size_t i = 0; i < n; ++i) { \
		std::string s = run_rich(1); \
        test_ds<TextDS<>>(s, &testfunc); \
	}

#define TEST_RANDOM(testfunc, n) \
	for(size_t i = 2; i < n; ++i) { \
	 	for(size_t j = 0; j < 2+50/(i+1); ++j) { \
	 		std::string s = random_uniform(1<<i,Ranges::numbers,j); \
	 		test_ds<TextDS<>>(s, &testfunc); \
	 	} \
	 }

#define TEST_ALL(testfunc) \
    TEST_COLLECTION(testfunc); \
    TEST_FIBONACCI(testfunc, 20); \
    TEST_THUEMORSE(testfunc, 20); \
    TEST_RUNRICH(testfunc, 20); \
    TEST_RANDOM(testfunc, 11);

void test_lcpsada(const std::string&,TextDS<>& t) {
	lcp_sada<TextDS<>,SuffixArray<TextDS<>>> lcp;
	lcp.construct(t);
	std::cout << lcp.size() << std::endl;
	assert_eq_sequence(lcp, t.require_lcp());
}
TEST(ds, lcpsada) { TEST_ALL(test_lcpsada); }

TEST(ds, SA)          { TEST_ALL(test_sa); }
TEST(ds, ISA)         { TEST_ALL(test_isa); }
TEST(ds, Phi)         { TEST_ALL(test_phi); }
TEST(ds, LCP)         { TEST_ALL(test_lcp); }
TEST(ds, Integration) { TEST_ALL(test_all_ds); }
