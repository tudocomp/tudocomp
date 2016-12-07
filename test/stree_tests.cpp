
#include "gtest/gtest.h"

#include "tudocomp_test_util.hpp"

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

    tdc::SuffixTree::STNode* root = stree->get_root();
    DLOG(INFO) <<"sizeof childnodes root "<< root->child_nodes.size();
    auto it = root->child_nodes.begin();
    while(it != root->child_nodes.end()){
        auto pair = *it;
        SuffixTree::STEdge* edge = pair.second;
        DLOG(INFO)<< pair.first <<" " <<edge->end;
        it++;
    }

    ASSERT_FALSE(false);
    ASSERT_TRUE(true);
}
