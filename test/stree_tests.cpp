
#include "gtest/gtest.h"

#include "test/util.hpp"

#include "tudocomp/ds/SuffixTree.hpp"


//#include "tudocomp/ds/SuffixTreeEdge.hpp"
//#include "tudocomp/ds/SuffixTreeNode.hpp"

//using namespace tdc;
using tdc::SuffixTree;

TEST(stree, st_node_test){
    //tdc::SuffixTree*
    tdc::SuffixTree* stree = new tdc::SuffixTree();

//abcabxabcd$
    stree->add_string("abcabxabcd$");
    ASSERT_EQ(stree->get_text(), "abcabxabcd$");

    tdc::SuffixTree::STNode* root = stree->get_root();
    ASSERT_EQ(6, root->child_nodes.size());
    //check sanity of STREE
    tdc::SuffixTree::STNode* a_node = root->child_nodes['a'];
    ASSERT_EQ(2,a_node->child_nodes.size());
    ASSERT_EQ("ab", stree->get_string_of_edge(a_node));


    tdc::SuffixTree::STNode* b_node = root->child_nodes['b'];

    //sl of a_ndoe should point at b_node:
    ASSERT_EQ(a_node->suffix_link,b_node);


    tdc::SuffixTree::STNode* abc_node = a_node->child_nodes['c'];
    tdc::SuffixTree::STNode* bc_node = b_node->child_nodes['c'];

    ASSERT_EQ(abc_node->suffix_link,bc_node);


    std::vector<tdc::SuffixTree::STNode*> leaves = stree->get_leaves();

    for(int i =0; i<leaves.size(); i++){
        DLOG(INFO)<<stree->get_text().substr(leaves[i]->suffix);
    }


    /*for(char c : {'a','b','c','d','x','$'}){//'d'

        tdc::SuffixTree::STNode* x = root->child_nodes[c];
        uint x_start = x->start;
        uint x_end = x->end;
        if(x_end==0){
            x_end=10;
        }
        DLOG(INFO) <<"sizeof childnodes "<< c<<": "<< x->child_nodes.size();
        DLOG(INFO) << "subst of "<< x_start<< "  " << x_end;
        DLOG(INFO) << "subst of "<< c<< ": "<< stree->get_string_of_edge(x);
    }
   tdc::SuffixTree::STNode* a = root->child_nodes['a'];

   for(char c : {'c','x'}){//'d'

        tdc::SuffixTree::STNode* x = a->child_nodes[c];
        uint x_start = x->start;
        uint x_end = x->end;
        if(x_end==0){
            x_end=10;
        }
        DLOG(INFO) <<"sizeof childnodes "<< c<<": "<< x->child_nodes.size();
        DLOG(INFO) << "subst of "<< x_start<< "  " << x_end;
        DLOG(INFO) << "subst of "<< c<< ": "<< stree->get_string_of_edge(x);
    }*/

    //ASSERT_FALSE(false);
    //ASSERT_TRUE(true);
}
