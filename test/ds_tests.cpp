#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/bwt.hpp>
#include <tudocomp/ds/SparseISA.hpp>
#include <tudocomp/ds/CompressedLCP.hpp>
#include "test/util.hpp"

using namespace tdc;
using io::InputView;

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
void test_bwt(const View& str, textds_t& t) {
    auto& sa = t.require_sa();
	const size_t size = t.size();

	std::vector<uliteral_t> bwt(size);
	for(size_t i = 0; i < size; ++i) {
		bwt[i] = bwt::bwt(t, sa, i);
	}

	auto decoded_string = bwt::decode_bwt(bwt);
	for(size_t i = 0; i < size; ++i) {
    	ASSERT_EQ(uliteral_t(decoded_string[i]), t[i]);
    }
}


template<class textds_t>
void test_sa(const View& str, textds_t& t) {
    auto& sa = t.require_sa();
	const size_t size = t.size();

    ASSERT_EQ(sa.size(), size); //length
    assert_permutation(sa, size); //permutation
    ASSERT_EQ(sa[0], size-1); //first element is $

    //lexicographic order
    for(size_t i = 1; i < size; i++) {
        ASSERT_GE(t[sa[i]], t[sa[i-1]]);
		ASSERT_LT(str.substr(sa[i-1]), str.substr(sa[i]));
		ASSERT_LT(View(str).substr(sa[i-1]), View(str).substr(sa[i]));
    }
}

template<class textds_t>
void test_isa(const View&, textds_t& t) {
    auto& isa = t.require_isa();
    auto& sa  = t.require_sa(); //request afterwards!

    ASSERT_EQ(isa.size(), sa.size()); //length

    //correctness
	for(size_t i = 0; i < sa.size(); ++i) {
		ASSERT_EQ(isa[sa[i]], i);
	}
}

template<class textds_t>
void test_lcp(const View& str, textds_t& t) {
    auto& lcp = t.require_lcp();
    auto& sa  = t.require_sa(); //request afterwards!

    ASSERT_EQ(lcp.size(), sa.size()); //length

    //correctness
	for(size_t i = 1; i < lcp.size(); ++i) {
		ASSERT_EQ(lcp[i], longest_common_extension(str, sa[i], sa[i-1]));
	}
}

template<class textds_t>
void test_all_ds(const View& str, textds_t& t) {
    test_sa(str, t);
    test_bwt(str,t);
    test_lcp(str, t);
    test_isa(str, t);
}

template<class textds_t>
class RunTestDS {
	void (*m_testfunc)(const View&, textds_t&);
	public:
	RunTestDS(void (*testfunc)(const View&, textds_t&))
		: m_testfunc(testfunc) {}

	void operator()(const std::string& str) {
		VLOG(2) << "str = \"" << str << "\"" << " size: " << str.length();
		test::TestInput input = test::compress_input(str);
		InputView in = input.as_view();
		auto t = Algorithm::instance<textds_t>("", in);
		m_testfunc(in, *t);
	}
};

#define TEST_DS_STRINGCOLLECTION(textds_t, func) \
	RunTestDS<textds_t> runner(func); \
	test::roundtrip_batch(runner); \
	test::on_string_generators(runner,11);

using textds_default_t = TextDS<>;
TEST(ds, default_SA)          { TEST_DS_STRINGCOLLECTION(textds_default_t, test_sa); }
TEST(ds, default_BWT)         { TEST_DS_STRINGCOLLECTION(textds_default_t, test_bwt); }
TEST(ds, default_LCP)         { TEST_DS_STRINGCOLLECTION(textds_default_t, test_lcp); }
TEST(ds, default_ISA)         { TEST_DS_STRINGCOLLECTION(textds_default_t, test_isa); }
TEST(ds, default_Integration) { TEST_DS_STRINGCOLLECTION(textds_default_t, test_all_ds); }

using textds_sparse_isa_t = TextDS<
    SADivSufSort, PhiFromSA, PLCPFromPhi, LCPFromPLCP, SparseISA<SADivSufSort>>;

TEST(ds, sparse_isa_ISA)         { TEST_DS_STRINGCOLLECTION(textds_sparse_isa_t, test_isa); }
TEST(ds, sparse_isa_Integration) { TEST_DS_STRINGCOLLECTION(textds_sparse_isa_t, test_all_ds); }

using textds_comp_lcp_t = TextDS<
    SADivSufSort, PhiFromSA, PLCPFromPhi, CompressedLCP<SADivSufSort>, ISAFromSA>;

TEST(ds, comp_lcp_LCP)         { TEST_DS_STRINGCOLLECTION(textds_comp_lcp_t, test_lcp); }
TEST(ds, comp_lcp_Integration) { TEST_DS_STRINGCOLLECTION(textds_comp_lcp_t, test_all_ds); }
