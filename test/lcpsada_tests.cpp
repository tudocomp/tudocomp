#include <tudocomp/config.h>

#ifdef SDSL_FOUND

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tudocomp/io.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/bwt.hpp>
#include "test/util.hpp"
#include <tudocomp/ds/LCPSada.hpp>

#include <tudocomp/ds/DSManager.hpp>
#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/ds/providers/PhiFromSA.hpp>
#include <tudocomp/ds/providers/PhiAlgorithm.hpp>
#include <tudocomp/ds/providers/LCPFromPLCP.hpp>

using namespace tdc;
using io::InputView;

template<typename textds_t>
void test_lcpsada(textds_t& t) {
    t.template construct<
        ds::SUFFIX_ARRAY,
        ds::PHI_ARRAY,
        ds::PLCP_ARRAY,
        ds::LCP_ARRAY>();

	auto& sa = t.template get<ds::SUFFIX_ARRAY>();
	typedef typename std::remove_reference<decltype(sa)>::type sa_t;
	typedef DynamicIntVector phi_t;
	phi_t phi { construct_phi_array<phi_t,sa_t>(sa) };
	test::assert_eq_sequence(phi, t.template get<ds::PHI_ARRAY>());
	auto lcp = construct_lcp_sada(sa, t.input);
	test::assert_eq_sequence(lcp, t.template get<ds::LCP_ARRAY>());
	LCPForwardIterator plcp { construct_plcp_bitvector(sa, t.input) };
	auto& plcp_ds = t.template get<ds::PLCP_ARRAY>();
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
		auto t = Algorithm::instance<textds_t>("", in);
		m_testfunc(*t);
	}
};

using textds_t = DSManager<DivSufSort, PhiFromSA, PhiAlgorithm, LCPFromPLCP>;

#define TEST_DS_STRINGCOLLECTION(func) \
	RunTestDS<textds_t> runner(func); \
	test::roundtrip_batch(runner); \
	test::on_string_generators(runner,14);
TEST(ds, lcpsada)          { TEST_DS_STRINGCOLLECTION(test_lcpsada); }
#undef TEST_DS_STRINGCOLLECTION

#endif // SDSL_FOUND

