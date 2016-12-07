
#include "gtest/gtest.h"
#include "tudocomp/ds/SuffixTree.hpp"


//#include "tudocomp/ds/SuffixTreeEdge.hpp"
//#include "tudocomp/ds/SuffixTreeNode.hpp"

//using namespace tdc;

TEST(stree, st_node_test){
    tdc::SuffixTree* stree = new tdc::SuffixTree();

    stree->add_char('h');

    stree->add_string("ello world");
    stree->add_char('!');
    ASSERT_EQ(stree->get_text(), "hello world!");

    ASSERT_FALSE(false);
    ASSERT_TRUE(true);
}
