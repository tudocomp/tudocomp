#include <gtest/gtest.h>

#include <sdsl/cst_sada.hpp>
#include <tudocomp/ds/SuffixTree.hpp>
#include "test/util.hpp"

using namespace tdc;

void test_strdepth(const std::string& str) {
	if(str.length() == 0) return;
	cst_sada<> cst;
	construct_im(cst, str, 1);
    SuffixTree st(cst);
	root_childrank_support rrank(cst.bp_support);
	for(auto node : cst) {
		ASSERT_EQ(st.str_depth(node), rrank.str_depth(st,node));
	}
}

TEST(SuffixTree, strdepth)     { 
	test::roundtrip_batch(test_strdepth);
	test::on_string_generators(test_strdepth,11);
}
