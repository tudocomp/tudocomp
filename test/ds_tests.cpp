#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tudocomp/io.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/bwt.hpp>
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


template<class textds_t>
void test_bwt(const std::string& str, textds_t& t) {
    auto& sa = t.require_sa();

	const len_t input_size = str.length()+1;
	std::vector<char> bwt;
	for(size_t i = 0; i < input_size; ++i) {
		bwt.push_back(bwt::bwt(str,sa,i));
	}      
	bwt::char_t* decoded_string = bwt::decode_bwt(bwt);
	if(decoded_string == nullptr) {
		ASSERT_EQ(str.length(), 0);
		return;
	}
	std::string decoded;
	decoded.assign(reinterpret_cast<char*>(decoded_string));
	ASSERT_EQ(decoded, str);
	delete [] decoded_string;
}


// === THE ACTUAL TESTS ===
template<class textds_t>
void test_sa(const std::string& str, textds_t& t) {
    auto& sa = t.require_sa();
	const size_t size = t.size();

    ASSERT_EQ(sa.size(), size); //length
    assert_permutation(sa, size); //permutation
    ASSERT_EQ(sa[0], size-1); //first element is $

    //lexicographic order
    for(size_t i = 1; i < size; i++) {
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
	test_bwt(str,t);
    test_sa(str, t);
    test_isa(str, t);
    test_phi(str, t);
    test_lcp(str, t);
    test_lcp_naive(str, t);
}


void test_lcpsada(const std::string&,TextDS<>& t) {
	lcp_sada<TextDS<>> lcp;
	lcp.construct(t);
	std::cout << lcp.size() << std::endl;
	assert_eq_sequence(lcp, t.require_lcp());
}





template<class textds_t>
class RunTestDS {
	void (*m_testfunc)(const std::string&, textds_t&);
	public:
	RunTestDS(void (*testfunc)(const std::string&, textds_t&)) 
		: m_testfunc(testfunc) {}

	void operator()(const std::string& str) {
		VLOG(2) << "str = \"" << str << "\"" << " size: " << str.length();
		Input input(str);
		InputView in = input.as_view();
		in.ensure_null_terminator();
		DCHECK_EQ(str.length()+1, in.size());
		textds_t t(in);
		DCHECK_EQ(str.length()+1, t.size());
		m_testfunc(str, t);
	}
};

#define TEST_DS_STRINGCOLLECTION(func) \
	RunTestDS<TextDS<>> runner(test_sa); \
	test_roundtrip_batch(runner); \
	test_on_string_generators(runner,11); 
TEST(ds, lcpsada)     { TEST_DS_STRINGCOLLECTION(test_lcpsada); }
TEST(ds, SA)          { TEST_DS_STRINGCOLLECTION(test_sa); }
TEST(ds, ISA)         { TEST_DS_STRINGCOLLECTION(test_isa); }
TEST(ds, Phi)         { TEST_DS_STRINGCOLLECTION(test_phi); }
TEST(ds, LCP)         { TEST_DS_STRINGCOLLECTION(test_lcp); }
TEST(ds, Integration) { TEST_DS_STRINGCOLLECTION(test_all_ds); }
#undef TEST_DS_STRINGCOLLECTION

