#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/bwt.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include "test/util.hpp"
#include <tudocomp/ds/LCPSada.hpp>

using namespace tdc;
using io::InputView;


void test_lcpsada(TextDS<>& t) {
	auto& sa = t.require_sa();
	typedef std::remove_reference<decltype(sa)>::type sa_t;
	typedef DynamicIntVector phi_t;
	phi_t phi { construct_phi_array<phi_t,sa_t>(sa) };
	test::assert_eq_sequence(phi, t.require_phi());
	auto lcp = construct_lcp_sada(t.env(), sa, t);
	test::assert_eq_sequence(lcp, t.require_lcp());
	LCPForwardIterator plcp { construct_plcp_bitvector(t.env(), sa, t) };
	auto& plcp_ds = t.require_plcp();
	if(sa.size() > 1)
    for (size_t i = 0; i < sa.size()-1; i++) {
		DCHECK_EQ(plcp.index(),i);
        ASSERT_EQ(plcp_ds[i], plcp()) << "assert_eq_sequence: failed at i=" << i;
		plcp.advance(); 
	}
}



template<class textds_t>
class RunTestDS {
	void (*m_testfunc)(textds_t&);
	public:
	RunTestDS(void (*testfunc)(textds_t&))
		: m_testfunc(testfunc) {}

	void operator()(const std::string& str) {
		VLOG(2) << "str = \"" << str << "\"" << " size: " << str.length();
		test::TestInput input = test::compress_input(str);
		InputView in = input.as_view();
		DCHECK_EQ(str.length()+1, in.size());
		textds_t t = create_algo<textds_t>("", in);
		DCHECK_EQ(str.length()+1, t.size());
		m_testfunc(t);
	}
};


#define TEST_DS_STRINGCOLLECTION(func) \
	RunTestDS<TextDS<>> runner(func); \
	test::roundtrip_batch(runner); \
	test::on_string_generators(runner,14);
TEST(ds, lcpsada)          { TEST_DS_STRINGCOLLECTION(test_lcpsada); }
#undef TEST_DS_STRINGCOLLECTION

