#include <functional>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/ds/DSManager.hpp>

#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/ds/providers/ISAFromSA.hpp>
#include <tudocomp/ds/providers/SparseISA.hpp>
#include <tudocomp/ds/providers/PhiAlgorithm.hpp>
#include <tudocomp/ds/providers/PhiFromSA.hpp>
#include <tudocomp/ds/providers/LCPFromPLCP.hpp>

#include <tudocomp/ds/bwt.hpp>

#include "test/util.hpp"

using namespace tdc;

// naive, but certainly correct LCE computation
template<typename text_t>
size_t naive_lce(const text_t& text, size_t a, size_t b) {
	DCHECK_NE(a,b);
	size_t i = 0;
	while(text[a+i] == text[b+i]) { ++i; }
	return i;
}

template<typename ds_t>
void test_sa(const ds_t& ds) {
    auto& t = ds.input;
    auto& sa = ds.template get<ds::SUFFIX_ARRAY>();

	const size_t size = t.size();

    ASSERT_EQ(sa.size(), size); //length
    assert_permutation(sa, size); //permutation
    ASSERT_EQ(sa[0], size-1); //first element is $

    //lexicographic order
    for(size_t i = 1; i < size; i++) {
        ASSERT_GE(t[sa[i]], t[sa[i-1]]);
		ASSERT_LT(t.substr(sa[i-1]), t.substr(sa[i]));
		ASSERT_LT(t.substr(sa[i-1]), t.substr(sa[i]));
    }
}


template<typename ds_t>
void test_lcp(const ds_t& ds) {
    auto& sa = ds.template get<ds::SUFFIX_ARRAY>();
    auto& lcp = ds.template get<ds::LCP_ARRAY>();

    ASSERT_EQ(lcp.size(), sa.size()); //length

    //correctness
	for(size_t i = 1; i < lcp.size(); ++i) {
		ASSERT_EQ(lcp[i], naive_lce(ds.input, sa[i], sa[i-1]));
	}
}

template<typename ds_t>
void test_isa(const ds_t& ds) {
    auto& sa = ds.template get<ds::SUFFIX_ARRAY>();
    auto& isa = ds.template get<ds::INVERSE_SUFFIX_ARRAY>();

    ASSERT_EQ(isa.size(), sa.size()); //length

    //correctness
	for(size_t i = 0; i < sa.size(); ++i) {
		ASSERT_EQ(isa[sa[i]], i);
	}
}

template<typename ds_t>
void test_bwt(const ds_t& ds) {
    auto& t = ds.input;
    auto& sa = ds.template get<ds::SUFFIX_ARRAY>();
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
void test_all_ds(const textds_t& ds) {
    test_sa(ds);
    test_bwt(ds);
    test_lcp(ds);
    test_isa(ds);
}

template<typename ds_t, dsid_t... construct>
class RunTestDS {
private:
    typedef void (*testfunc_t)(const ds_t&);
	testfunc_t m_testfunc;

public:
	RunTestDS(testfunc_t testfunc) : m_testfunc(testfunc) {
    }

	void operator()(const std::string& str) {
		auto input = test::compress_input(str);
		auto view = input.as_view();        
        
        auto ds = Algorithm::instance<ds_t>(view);
        ds->template construct<construct...>();

		m_testfunc(*ds);
	}
};

#define TEST_DS_STRINGCOLLECTION(ds_t, func, ...) \
	RunTestDS<ds_t, __VA_ARGS__> runner(func); \
	test::roundtrip_batch(runner); \
	test::on_string_generators(runner,11);

using ds_default_t = DSManager<
    DivSufSort, PhiAlgorithm, LCPFromPLCP, ISAFromSA, PhiFromSA>;

TEST(ds, default_SA)          { TEST_DS_STRINGCOLLECTION(ds_default_t, test_sa, ds::SUFFIX_ARRAY ); }
TEST(ds, default_BWT)         { TEST_DS_STRINGCOLLECTION(ds_default_t, test_bwt, ds::SUFFIX_ARRAY); }
TEST(ds, default_LCP)         { TEST_DS_STRINGCOLLECTION(ds_default_t, test_lcp, ds::SUFFIX_ARRAY, ds::LCP_ARRAY ); }
TEST(ds, default_ISA)         { TEST_DS_STRINGCOLLECTION(ds_default_t, test_isa, ds::SUFFIX_ARRAY, ds::INVERSE_SUFFIX_ARRAY); }
TEST(ds, default_Integration) { TEST_DS_STRINGCOLLECTION(ds_default_t, test_all_ds, ds::SUFFIX_ARRAY, ds::LCP_ARRAY, ds::INVERSE_SUFFIX_ARRAY ); }

using ds_sparse_isa_t = DSManager<
    DivSufSort, PhiAlgorithm, LCPFromPLCP, SparseISA<DivSufSort>, PhiFromSA>;

TEST(ds, sparse_isa_ISA)         { TEST_DS_STRINGCOLLECTION(ds_sparse_isa_t, test_isa, ds::SUFFIX_ARRAY, ds::INVERSE_SUFFIX_ARRAY); }
TEST(ds, sparse_isa_Integration) { TEST_DS_STRINGCOLLECTION(ds_sparse_isa_t, test_all_ds, ds::SUFFIX_ARRAY, ds::LCP_ARRAY, ds::INVERSE_SUFFIX_ARRAY ); }

