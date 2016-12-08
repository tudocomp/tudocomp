
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
   // ASSERT_EQ(stree->get_text(), "abcabxabcd$");

    tdc::SuffixTree::STNode* root = stree->get_root();
    DLOG(INFO) <<"sizeof childnodes root "<< root->child_nodes.size();
    //check sanity of STREE
   for(char c : {'a','b','c','d','x','$'}){//'d'

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
    }

    //ASSERT_FALSE(false);
    ASSERT_TRUE(true);
}
