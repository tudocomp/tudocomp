
#include "gtest/gtest.h"

#include "test/util.hpp"

#include "tudocomp/ds/SuffixTree.hpp"

#include "tudocomp/ds/BST.hpp"

#include "tudocomp/ds/BinarySuffixTree.hpp"


#include <tudocomp/io.hpp>


//#include "tudocomp/ds/SuffixTreeEdge.hpp"
//#include "tudocomp/ds/SuffixTreeNode.hpp"

namespace tdc {
//namespace SuffixTree {

BST<> * build_suffix_tree(Input& input){
    //input.escape_and_terminate();
    io::InputView file_view = input.as_view();

    tdc::BST<> * stree = new tdc::BST<>(file_view);

   // stree->append_input(input);
    return stree;
}

BinarySuffixTree * build_bin_suffix_tree(Input& input){
    //input.escape_and_terminate();
    io::InputView file_view = input.as_view();

    tdc::BinarySuffixTree * stree = new tdc::BinarySuffixTree(file_view);
   // stree->print_tree(0, "");
   // stree->append_input(input);
    return stree;
}



TEST(stree, st_input_test_small){
    Input file_input("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum.$");
   // BST<> * stree = build_suffix_tree(file_input);

    //stree->print_tree(0, "");

  //  BinarySuffixTree * stree2 = build_bin_suffix_tree(file_input);

 //   stree2->print_tree(0, "");
    /*
    DLOG(INFO)<<"childs of root:";
    uint child  = stree->get_first_child(0);
    if(child != 0){
        do{
            DLOG(INFO)<<stree->get_string_of_edge(child) << " length: " << stree->edge_length(child);
        }
        while ((child = stree->get_next_sibling(child)) != 0);
    }*/
  //   DLOG(INFO)<<"size of tree1: " << stree->get_tree_size();

   //  DLOG(INFO)<<"size of tree2: " << stree2->get_tree_size();


   // auto leaves = stree->get_leaves();
  //  ASSERT_EQ(stree->get_text().size(), file_input.size());
}

/*
TEST(stree, bin_st_file_test_1mb_english){
    Input file_input = test::TestInput(Path{"english.1MB"}, true);

    //test::TestInput file_input = test::compress_input_file("english.1MB");
    BinarySuffixTree * stree = build_bin_suffix_tree(file_input);

    DLOG(INFO)<<"size of tree: " << stree->get_tree_size();
 //   ASSERT_EQ(stree->get_size(), file_input.size());

}*/

TEST(stree, int_st_file_test_1mb_english){
    Input file_input = test::TestInput(Path{"english.1MB"}, true);

    //test::TestInput file_input = test::compress_input_file("english.1MB");
    BST<>* stree = build_suffix_tree(file_input);

    DLOG(INFO)<<"size of tree: " << stree->get_tree_size();
 //   ASSERT_EQ(stree->get_size(), file_input.size());

}

/*

TEST(stree, st_node_test){
    SuffixTree* stree = new SuffixTree();
    stree->append_string("abcabxabcd$");
    ASSERT_EQ(stree->get_text(), "abcabxabcd$");

    SuffixTree::STNode* root = stree->get_root();
    ASSERT_EQ(6, root->child_nodes.size());
    //check sanity of STREE
    SuffixTree::STNode* a_node = root->child_nodes['a'];
    ASSERT_EQ(2,a_node->child_nodes.size());
    ASSERT_EQ("ab", stree->get_string_of_edge(a_node));


    SuffixTree::STNode* b_node = root->child_nodes['b'];

    //sl of a_ndoe should point at b_node:
    ASSERT_EQ(a_node->suffix_link,b_node);


    SuffixTree::STNode* abc_node = a_node->child_nodes['c'];
    SuffixTree::STNode* bc_node = b_node->child_nodes['c'];

    ASSERT_EQ(abc_node->suffix_link,bc_node);

    SuffixTree::STNode* check;
    SuffixTree::STNode* check_l;
    SuffixTree::STNode* check_r;
    //check $
    check =root->child_nodes['$'];
    ASSERT_EQ("$", stree->get_string_of_edge(check));

    ////////////////////////////////////////////////
    //check a

    check =root->child_nodes['a'];
    ASSERT_EQ("ab", stree->get_string_of_edge(check));
    ASSERT_EQ(2, check->child_nodes.size());

    //check children of a

    check_l=check->child_nodes['c'];
    check_r=check->child_nodes['x'];

    ASSERT_EQ("c", stree->get_string_of_edge(check_l));
    ASSERT_EQ("xabcd$", stree->get_string_of_edge(check_r));

    // check children of c of a:
    check = check_l;

    check_l=check->child_nodes['a'];
    check_r=check->child_nodes['d'];

    ASSERT_EQ("abxabcd$", stree->get_string_of_edge(check_l));
    ASSERT_EQ("d$", stree->get_string_of_edge(check_r));

    ////////////////////////////////////////////////
    //check b of root:

    check =root->child_nodes['b'];

    ASSERT_EQ("b", stree->get_string_of_edge(check));
    ASSERT_EQ(2, check->child_nodes.size());

    //check children of b

    check_l=check->child_nodes['c'];
    check_r=check->child_nodes['x'];

    ASSERT_EQ("c", stree->get_string_of_edge(check_l));
    ASSERT_EQ("xabcd$", stree->get_string_of_edge(check_r));

    // check children of c of b:
    check = check_l;

    check_l=check->child_nodes['a'];
    check_r=check->child_nodes['d'];

    ASSERT_EQ("abxabcd$", stree->get_string_of_edge(check_l));
    ASSERT_EQ("d$", stree->get_string_of_edge(check_r));

    ////////////////////////////////////////////////
    //check c of root:

    check =root->child_nodes['c'];

    ASSERT_EQ("c", stree->get_string_of_edge(check));
    ASSERT_EQ(2, check->child_nodes.size());

    //check children of c

    check_l=check->child_nodes['a'];
    check_r=check->child_nodes['d'];

    ASSERT_EQ("abxabcd$", stree->get_string_of_edge(check_l));
    ASSERT_EQ("d$", stree->get_string_of_edge(check_r));

    ////////////////////////////////////////////////
    //check d of root:

    check =root->child_nodes['d'];

    ASSERT_EQ("d$", stree->get_string_of_edge(check));
    ASSERT_EQ(0, check->child_nodes.size());

    ////////////////////////////////////////////////
    //check x of root:

    check =root->child_nodes['x'];

    ASSERT_EQ("xabcd$", stree->get_string_of_edge(check));
    ASSERT_EQ(0, check->child_nodes.size());


}

TEST(stree, st_input_test_small){
    Input file_input("abcabxabcd$abcabxabcd$abcabxabcd$abcabxabcd$");
    SuffixTree* stree = build_suffix_tree(file_input);
    auto leaves = stree->get_leaves();
    ASSERT_EQ(stree->get_text().size(), leaves.size());
}

TEST(stree, st_input_test_large){
    Input file_input("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod "
                     "tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At "
                     "vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, "
                     "no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet,"
                     " consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et "
                     "dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo "
                     "dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem "
                     "ipsum dolor sit amet.$");
    SuffixTree* stree = build_suffix_tree(file_input);
    auto leaves = stree->get_leaves();
    ASSERT_EQ(stree->get_text().size(), leaves.size());

}


TEST(stree, st_constructor_tests){
    Input input ("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod "
                     "tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At "
                     "vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, "
                     "no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet,"
                     " consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et "
                     "dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo "
                     "dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem "
                     "ipsum dolor sit amet.$");

    SuffixTree stree (input);
    auto leaves = stree.get_leaves();
    ASSERT_EQ(stree.get_text().size(), leaves.size());


    std::string str_in = "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod "
                         "tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At "
                         "vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, "
                         "no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet,"
                         " consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et "
                         "dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo "
                         "dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem "
                         "ipsum dolor sit amet.$";
    SuffixTree stree_str (str_in);

    leaves = stree_str.get_leaves();
    ASSERT_EQ(stree_str.get_text().size(), leaves.size());
}

TEST(stree, st_file_test_1mb_english){
    Input file_input = Input(Input::Path{"english.1MB"});
    SuffixTree* stree = build_suffix_tree(file_input);
    auto leaves = stree->get_leaves();
    ASSERT_EQ(stree->get_text().size(), leaves.size());

}

TEST(stree, st_file_test_1mb_sources){
    Input file_input = Input(Input::Path{"sources.1MB"});
    SuffixTree* stree = build_suffix_tree(file_input);
    auto leaves = stree->get_leaves();
    ASSERT_EQ(stree->get_text().size(), leaves.size());

}
TEST(stree, st_file_test_10mb_english){
    Input file_input = Input(Input::Path{"english.10MB"});
    SuffixTree* stree = build_suffix_tree(file_input);
    auto leaves = stree->get_leaves();
    ASSERT_EQ(stree->get_text().size(), leaves.size());

}

TEST(stree, st_file_test_10mb_sources){
    Input file_input = Input(Input::Path{"sources.10MB"});
    SuffixTree* stree = build_suffix_tree(file_input);
    auto leaves = stree->get_leaves();
    ASSERT_EQ(stree->get_text().size(), leaves.size());

}  */



}

