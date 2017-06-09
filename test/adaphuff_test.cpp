#include <utility>
#include "test/util.hpp"
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <tudocomp/coders/AdaptiveHuffmanCoder.hpp>

void test_basic_tree_creation() {
    using namespace tdc::adaphuff;

    codingtree test_codingtree;

    std::vector<bool> dictionary(SYMBOL, false);

    test_codingtree.dictionary = &dictionary;
    test_codingtree.root = nullptr;
    test_codingtree.nyt = nullptr;

    node* root_pointer;

    //create nyt root
    uliteral_t symbol = 'a';
    create_node(&root_pointer, symbol, true);
    test_codingtree.root = root_pointer;
    test_codingtree.nyt = root_pointer;
    ASSERT_TRUE(test_codingtree.root->is_leaf());


    //add one node with symbol a
    //use merge
    node* new_node;
    create_node(&new_node, symbol, false);
    merge_node(&root_pointer, root_pointer, new_node);


//    std::string tree_as_string;
//    tree_to_string(test_codingtree.root, tree_as_string);
//    std::cout << tree_as_string;

    delete_tree(&root_pointer);
}

inline void init_codingtree(tdc::adaphuff::codingtree* pCodingTree, std::vector<bool>* pDictionary) {
    pCodingTree->dictionary = pDictionary;
    pCodingTree->nyt = nullptr;
    pCodingTree->root = nullptr;
}

inline void delete_codingtree(tdc::adaphuff::codingtree* codingtree) {
    delete codingtree->dictionary;

    tdc::adaphuff::delete_tree(&(codingtree->root));

    codingtree->root = nullptr;
    codingtree->nyt = nullptr;
}


void create_test_tree(tdc::adaphuff::codingtree& test_codingtree) {
    using namespace tdc::adaphuff;

    std::vector<bool>* dictionary = new std::vector<bool>(SYMBOL, false);
    init_codingtree(&test_codingtree, dictionary);

    node* node_pointer[15];

    for (unsigned int i=0; i<15; i++) {
        node* new_node = new node;
        new_node->number = NUMBER-(14-i);
        node_pointer[i] = new_node;
    }

    test_codingtree.root = node_pointer[14];
    test_codingtree.nyt = node_pointer[0];

    //Test-string is "e eae de eabe eae dcf"
    //see: https://www.ics.uci.edu/~dan/pubs/DC-fig46.gif
    std::string test_string = "e eae de eabe eae dcf";

    for (unsigned char character = 0x00; character < 0xff; character ++) {
        auto pos_in_dictionary = static_cast<unsigned int>(character);
        if (test_string.find(character) != std::string::npos) {
            dictionary->at(pos_in_dictionary) = true;
        }
    }

    unsigned char character = 0xff;
    auto pos_in_dictionary = static_cast<unsigned int>(character);
    if (test_string.find(character) != std::string::npos) {
        dictionary->at(pos_in_dictionary) = true;
    }


    node_pointer[0]->weight = 0;
    node_pointer[0]->symbol = 0x00;
    node_pointer[0]->parent = node_pointer[4];
    node_pointer[0]->left = nullptr;
    node_pointer[0]->right = nullptr;

    node_pointer[1]->weight = 1;
    node_pointer[1]->symbol = 'f';
    node_pointer[1]->parent = node_pointer[4];
    node_pointer[1]->left = nullptr;
    node_pointer[1]->right = nullptr;

    node_pointer[2]->weight = 1;
    node_pointer[2]->symbol = 'c';
    node_pointer[2]->parent = node_pointer[6];
    node_pointer[2]->left = nullptr;
    node_pointer[2]->right = nullptr;

    node_pointer[3]->weight = 1;
    node_pointer[3]->symbol = 'b';
    node_pointer[3]->parent = node_pointer[6];
    node_pointer[3]->left = nullptr;
    node_pointer[3]->right = nullptr;

    node_pointer[4]->weight = 1;
    node_pointer[4]->symbol = 0x00;
    node_pointer[4]->parent = node_pointer[8];
    node_pointer[4]->left = node_pointer[0];
    node_pointer[4]->right = node_pointer[1];

    node_pointer[5]->weight = 2;
    node_pointer[5]->symbol = 'd';
    node_pointer[5]->parent = node_pointer[8];
    node_pointer[5]->left = nullptr;
    node_pointer[5]->right = nullptr;

    node_pointer[6]->weight = 2;
    node_pointer[6]->symbol = 0x00;
    node_pointer[6]->parent = node_pointer[10];
    node_pointer[6]->left = node_pointer[2];
    node_pointer[6]->right = node_pointer[3];

    node_pointer[7]->weight = 3;
    node_pointer[7]->symbol = 'a';
    node_pointer[7]->parent = node_pointer[10];
    node_pointer[7]->left = nullptr;
    node_pointer[7]->right = nullptr;

    node_pointer[8]->weight = 3;
    node_pointer[8]->symbol = 0x00;
    node_pointer[8]->parent = node_pointer[12];
    node_pointer[8]->left = node_pointer[4];
    node_pointer[8]->right = node_pointer[5];

    node_pointer[9]->weight = 5;
    node_pointer[9]->symbol = ' ';
    node_pointer[9]->parent = node_pointer[12];
    node_pointer[9]->left = nullptr;
    node_pointer[9]->right = nullptr;

    node_pointer[10]->weight = 5;
    node_pointer[10]->symbol = 0x00;
    node_pointer[10]->parent = node_pointer[13];
    node_pointer[10]->left = node_pointer[6];
    node_pointer[10]->right = node_pointer[7];

    node_pointer[11]->weight = 8;
    node_pointer[11]->symbol = 'e';
    node_pointer[11]->parent = node_pointer[13];
    node_pointer[11]->left = nullptr;
    node_pointer[11]->right = nullptr;

    node_pointer[12]->weight = 8;
    node_pointer[12]->symbol = 0x00;
    node_pointer[12]->parent = node_pointer[14];
    node_pointer[12]->left = node_pointer[8];
    node_pointer[12]->right = node_pointer[9];

    node_pointer[13]->weight = 13;
    node_pointer[13]->symbol = 0x00;
    node_pointer[13]->parent = node_pointer[14];
    node_pointer[13]->left = node_pointer[10];
    node_pointer[13]->right = node_pointer[11];

    node_pointer[14]->weight = 21;
    node_pointer[14]->symbol = 0x00;
    node_pointer[14]->parent = nullptr;
    node_pointer[14]->left = node_pointer[12];
    node_pointer[14]->right = node_pointer[13];
}

void create_test_tree_e(tdc::adaphuff::codingtree& test_codingtree) {
    using namespace tdc::adaphuff;

    std::vector<bool>* dictionary = new std::vector<bool>(SYMBOL, false);
    init_codingtree(&test_codingtree, dictionary);

    node* node_pointer[3];

    for (unsigned int i=0; i<3; i++) {
        node* new_node = new node;
        new_node->number = NUMBER-(2-i);
        node_pointer[i] = new_node;
    }

    test_codingtree.root = node_pointer[2];
    test_codingtree.nyt = node_pointer[0];

    std::string test_string = "e";

    for (unsigned char character = 0x00; character < 0xff; character ++) {
        auto pos_in_dictionary = static_cast<unsigned int>(character);
        if (test_string.find(character) != std::string::npos) {
            dictionary->at(pos_in_dictionary) = true;
        }
    }

    unsigned char character = 0xff;
    auto pos_in_dictionary = static_cast<unsigned int>(character);
    if (test_string.find(character) != std::string::npos) {
        dictionary->at(pos_in_dictionary) = true;
    }

    node_pointer[0]->weight = 0;
    node_pointer[0]->symbol = 0x00;
    node_pointer[0]->parent = node_pointer[2];
    node_pointer[0]->left = nullptr;
    node_pointer[0]->right = nullptr;

    node_pointer[1]->weight = 1;
    node_pointer[1]->symbol = 'e';
    node_pointer[1]->parent = node_pointer[2];
    node_pointer[1]->left = nullptr;
    node_pointer[1]->right = nullptr;

    node_pointer[2]->weight = 1;
    node_pointer[2]->symbol = 0x00;
    node_pointer[2]->parent = nullptr;
    node_pointer[2]->left = node_pointer[0];
    node_pointer[2]->right = node_pointer[1];
}

void create_test_tree_eSpace(tdc::adaphuff::codingtree& test_codingtree) {
    using namespace tdc::adaphuff;

    std::vector<bool>* dictionary = new std::vector<bool>(SYMBOL, false);
    init_codingtree(&test_codingtree, dictionary);

    node* node_pointer[5];

    for (unsigned int i=0; i<5; i++) {
        node* new_node = new node;
        new_node->number = NUMBER-(4-i);
        node_pointer[i] = new_node;
    }

    test_codingtree.root = node_pointer[4];
    test_codingtree.nyt = node_pointer[0];

    std::string test_string = "e ";

    for (unsigned char character = 0x00; character < 0xff; character ++) {
        auto pos_in_dictionary = static_cast<unsigned int>(character);
        if (test_string.find(character) != std::string::npos) {
            dictionary->at(pos_in_dictionary) = true;
        }
    }

    unsigned char character = 0xff;
    auto pos_in_dictionary = static_cast<unsigned int>(character);
    if (test_string.find(character) != std::string::npos) {
        dictionary->at(pos_in_dictionary) = true;
    }

    node_pointer[0]->weight = 0;
    node_pointer[0]->symbol = 0x00;
    node_pointer[0]->parent = node_pointer[3];
    node_pointer[0]->left = nullptr;
    node_pointer[0]->right = nullptr;

    node_pointer[1]->weight = 1;
    node_pointer[1]->symbol = ' ';
    node_pointer[1]->parent = node_pointer[3];
    node_pointer[1]->left = nullptr;
    node_pointer[1]->right = nullptr;

    node_pointer[2]->weight = 1;
    node_pointer[2]->symbol = 'e';
    node_pointer[2]->parent = nullptr;
    node_pointer[2]->left = nullptr;
    node_pointer[2]->right = nullptr;

    node_pointer[3]->weight = 1;
    node_pointer[3]->symbol = 0x00;
    node_pointer[3]->parent = node_pointer[4];
    node_pointer[3]->left = node_pointer[0];
    node_pointer[3]->right = node_pointer[1];

    node_pointer[4]->weight = 2;
    node_pointer[4]->symbol = 0x00;
    node_pointer[4]->parent = nullptr;
    node_pointer[4]->left = node_pointer[2];
    node_pointer[4]->right = node_pointer[3];
}

void reset_weight(tdc::adaphuff::node **root) {
    if (!(*root)->is_leaf()) {
        (*root)->weight = 0;

        if ((*root)->has_left()){
            reset_weight(&(*root)->left);
        }
        if ((*root)->has_right()){
            reset_weight(&(*root)->right);
        }
    }
}

void incr_weight(tdc::adaphuff::node **root){
    using namespace tdc::adaphuff;

    if ((*root)->has_left()){
        incr_weight(&(*root)->left);
    }

    if ((*root)->has_right()){
        incr_weight(&(*root)->right);
    }

    if (!(*root)->is_leaf()) {
        increment_weight(root);
    }
}

void compare_trees(tdc::adaphuff::node *tree1, tdc::adaphuff::node *tree2) {
    ASSERT_EQ(tree1->number, tree2->number);
    ASSERT_EQ(tree1->symbol, tree2->symbol);
    ASSERT_EQ(tree1->weight, tree2->weight);
    ASSERT_EQ(tree1->has_left(), tree2->has_left());
    ASSERT_EQ(tree1->has_right(), tree2->has_right());

    if (tree1->has_left()){
        compare_trees(tree1->left, tree2->left);
    }
    if (tree1->has_right()){
        compare_trees(tree1->right, tree2->right);
    }
}

void compare_trees_weight(tdc::adaphuff::node *tree1, tdc::adaphuff::node *tree2){
    ASSERT_EQ(tree1->weight, tree2->weight);
    ASSERT_EQ(tree1->has_left(), tree2->has_left());
    ASSERT_EQ(tree1->has_right(), tree2->has_right());

    if (tree1->has_left()){
        compare_trees_weight(tree1->left, tree2->left);
    }
    if (tree1->has_right()){
        compare_trees_weight(tree1->right, tree2->right);
    }
}

void compare_trees_number(tdc::adaphuff::node *tree1, tdc::adaphuff::node *tree2){
    ASSERT_EQ(tree1->number, tree2->number);
    ASSERT_EQ(tree1->has_left(), tree2->has_left());
    ASSERT_EQ(tree1->has_right(), tree2->has_right());

    if (tree1->has_left()){
        compare_trees_number(tree1->left, tree2->left);
    }
    if (tree1->has_right()){
        compare_trees_number(tree1->right, tree2->right);
    }
}


bool tree_has_property(tdc::adaphuff::node **root) {
    return false;
}

void test_existing_leaf(const uliteral_t literal) {
    using namespace tdc::adaphuff;

    codingtree test_codingtree;
    create_test_tree(test_codingtree);

    node *node = find_corresponding_leaf(&test_codingtree.root, literal);

    ASSERT_EQ(literal, node->symbol);

    delete_codingtree(&test_codingtree);
}

void test_no_existing_leaf(const uliteral_t literal) {
    using namespace tdc::adaphuff;

    codingtree test_codingtree;
    create_test_tree(test_codingtree);

    node *node = find_corresponding_leaf(&test_codingtree.root, literal);
    ASSERT_EQ(node, nullptr);

    delete_codingtree(&test_codingtree);
}

void test_increment_weight() {
    //TODO increment weight was changed to act like expected by wikipedia code
//    using namespace tdc::adaphuff;
//
//    codingtree test_codingtree;
//    create_test_tree(test_codingtree);
//
//    codingtree test_codingtree2;
//    create_test_tree(test_codingtree2);
//
//    reset_weight(&test_codingtree.root);
//    incr_weight(&test_codingtree.root);
//    compare_trees_weight(test_codingtree.root, test_codingtree2.root);
//
//    delete_codingtree(&test_codingtree);
//    delete_codingtree(&test_codingtree2);
}

void test_update(adaphuff::codingtree test_codingtree, std::string test_string) {
    using namespace tdc::adaphuff;

    codingtree update_codingtree;
    update_codingtree.root = nullptr;
    update_codingtree.nyt = nullptr;

    std::vector<bool> enc_dictionary(SYMBOL, false);
    update_codingtree.dictionary = &enc_dictionary;

    for (uliteral_t literal : test_string){
        update(&update_codingtree, literal);
    }

//    std::string tree_before = "";
//    tdc::adaphuff::tree_to_string(test_codingtree.root, tree_before);
//    std::cout << tree_before << std::endl;
//
//    std::string tree_after= "";
//    tdc::adaphuff::tree_to_string(update_codingtree.root, tree_after);
//    std::cout << tree_after << std::endl;

    compare_trees(test_codingtree.root, update_codingtree.root);

    delete_codingtree(&test_codingtree);
}

/*
 * Tests the basic tree creation operations
 */
TEST(adaphuff, tree_creation) {
    test_basic_tree_creation();
}

TEST (adaphuff, test_tree) {
    tdc::adaphuff::codingtree test_codingtree;
    create_test_tree(test_codingtree);

//    std::string tree_as_string;
//    tdc::adaphuff::tree_to_string(test_codingtree.root, tree_as_string);
//    std::cout << tree_as_string;


    delete_codingtree(&test_codingtree);
}

TEST (adaphuff, test_child_type) {
    tdc::adaphuff::codingtree test_codingtree;
    create_test_tree(test_codingtree);

    adaphuff::node *cur_node;
    char cur_child_type;

    cur_node = adaphuff::find_corresponding_leaf(&test_codingtree.root, 'f');
    cur_child_type = cur_node->child_type();
    ASSERT_EQ(cur_child_type, 'r');

    delete_codingtree(&test_codingtree);
}

TEST (adaphuff, basic_function_find_corresponding_leaf) {
    test_existing_leaf('a');
    test_existing_leaf(' ');
    test_existing_leaf('e');
    test_existing_leaf('f');
    test_no_existing_leaf('g');
    test_no_existing_leaf('u');
}

TEST (adaphuff, basic_function_increment_weigth) {
    // TODO Chris
    test_increment_weight();
}

TEST (adaphuff, basic_function_search_higher_block) {
    tdc::adaphuff::codingtree test_codingtree;
    create_test_tree(test_codingtree);

    const unsigned int weight_to_search_for = 1;
    unsigned int result_node_number = 0;
    const unsigned int parent_number = 5;
    bool result_lf;
    tdc::adaphuff::node* result_node_ancestor = nullptr;

    tdc::adaphuff::search_higher_block(&test_codingtree.root, weight_to_search_for, &result_node_number, parent_number, &result_node_ancestor, &result_lf);

    tdc::adaphuff::node* result_node;
    if (!result_lf) {
        result_node = (result_node_ancestor)->left;
    } else {
        result_node = (result_node_ancestor)->right;
    }

    ASSERT_EQ(result_node->number, 502);
    delete_codingtree(&test_codingtree);
}

inline tdc::adaphuff::node* get_child(tdc::adaphuff::node* ancestor, bool r_l) {
    if (!r_l) {
        return ancestor->left;
    } else {
        return ancestor->right;
    }
}

TEST (adaphuff, basic_function_switch_node) {
    tdc::adaphuff::codingtree test_codingtree;
    create_test_tree(test_codingtree);

//    std::string tree_before = "";
//    tdc::adaphuff::tree_to_string(test_codingtree.root, tree_before);
//    std::cout << tree_before << std::endl;

    //swap c and b
    //c node
    uliteral_t symbol_c = 'c';
    tdc::adaphuff::node* p_node_c = tdc::adaphuff::find_corresponding_leaf(&test_codingtree.root, symbol_c);
    bool l_r_c = false;
    //b node
    uliteral_t symbol_b = 'b';
    tdc::adaphuff::node* p_node_b = tdc::adaphuff::find_corresponding_leaf(&test_codingtree.root, symbol_b);
    bool l_r_b = true;

    tdc::adaphuff::switch_node(p_node_b->parent, &l_r_b, p_node_c->parent, &l_r_c);

//    std::string tree_after = "";
//    tdc::adaphuff::tree_to_string(test_codingtree.root, tree_after);
//    std::cout << tree_after << std::endl;

    delete_codingtree(&test_codingtree);
}

TEST (adaphuff, basic_function_numbering) {
    adaphuff::codingtree test_codingtree;
    create_test_tree(test_codingtree);

    adaphuff::codingtree test_codingtree2;
    create_test_tree(test_codingtree2);

//    std::string tree_before = "";
//    tdc::adaphuff::tree_to_string(test_codingtree.root, tree_before);
//    std::cout << tree_before << std::endl;

    std::vector<tdc::adaphuff::number_pair> queue;
    tdc::adaphuff::implicit_numbering(&test_codingtree);
    compare_trees_number(test_codingtree.root, test_codingtree2.root);

//    std::string tree_after= "";
//    tdc::adaphuff::tree_to_string(test_codingtree.root, tree_after);
//    std::cout << tree_after << std::endl;

    delete_codingtree(&test_codingtree);
    delete_codingtree(&test_codingtree2);
}

inline void slide (const unsigned int to_slide, const unsigned int sibling, const unsigned char sib_pos_after) {
    tdc::adaphuff::codingtree test_codingtree;
    create_test_tree(test_codingtree);

    tdc::adaphuff::node *p_node, *temp;

    tdc::adaphuff::find_node(test_codingtree.root, to_slide, p_node);
    temp = p_node;
    tdc::adaphuff::slide_and_increment(&test_codingtree.root, &temp);

    if (sib_pos_after == 'l') {
        ASSERT_EQ(p_node->parent->left->number, sibling);
    }
    if (sib_pos_after == 'r'){
        ASSERT_EQ(p_node->parent->right->number, sibling);
    }


    delete_codingtree(&test_codingtree);
}

TEST (adaphuff, function_slide_and_increment) {
    //internal slide
    //7 aka 504
    slide(504, 505, 'l');
    //11 aka 508
    slide(508, 508, 'l');

    //leaf slides
    //6 aka 503
    slide(503, 505, 'r');
    //11 aka 508
    slide(507, 509, 'r');
}


TEST (adaphuff, function_update_e) {
    using namespace tdc::adaphuff;

    codingtree test_codingtree;
    create_test_tree_e(test_codingtree);

    test_update(test_codingtree, "e");
}

TEST (adaphuff, function_update_eSpace) {
    using namespace tdc::adaphuff;

    codingtree test_codingtree;
    create_test_tree_eSpace(test_codingtree);

    test_update(test_codingtree, "e ");
}

TEST (adaphuff, function_update_e_eae_de_eabe_eae_dcf) {
    using namespace tdc::adaphuff;

    codingtree test_codingtree;
    create_test_tree(test_codingtree);

    test_update(test_codingtree, "e eae de eabe eae dcf");
}

TEST (adaphuff, function_update_2) {
    using namespace tdc::adaphuff;

    codingtree test_codingtree;
    create_test_tree(test_codingtree);

//    tdc::io::BitOStream *os = new tdc::io::BitOStream();
//
//    update(&update_codingtree, literal);
//
//    compare_trees(test_codingtree.root, update_codingtree.root);*/

    delete_codingtree(&test_codingtree);
    //delete_codingtree(&update_codingtree);
}

void queue_to_string(std::queue<bool>& q, std::string& s) {
    while(!q.empty()) {
        if(!q.front()) {
            s.append("0");
        } else {
            s.append("1");
        }
        q.pop();
    }
}

TEST (adaphuff, function_encode_gen_nyt_code) {
    tdc::adaphuff::codingtree test_codingtree;
    create_test_tree(test_codingtree);

    std::queue<bool> temp = std::queue<bool>();
    std::queue<bool> result = std::queue<bool>();

    tdc::adaphuff::gen_nyt_code(temp, result, test_codingtree.root);

    std::string code_should_be = "0000";
    std::string code_is = "";

    queue_to_string(result, code_is);

    ASSERT_TRUE(code_is.compare(code_should_be) == 0);

    delete_codingtree(&test_codingtree);
}

TEST (adaphuff, function_encode_gen_standard_code) {
    std::queue<bool> resultq_a;

    tdc::uliteral_t symbol_a = 'a';
    std::string code_a = "01100001";
    tdc::adaphuff::gen_standard_code(resultq_a, symbol_a);
    std::string result_a = "";
    queue_to_string(resultq_a, result_a);
    ASSERT_TRUE(result_a.compare(code_a) == 0);
}

TEST (adaphuff, function_encode_gen_code) {
    tdc::adaphuff::codingtree test_codingtree;
    create_test_tree(test_codingtree);

    std::queue<bool> temp = std::queue<bool>();
    std::queue<bool> result = std::queue<bool>();

    //Test for 'a'
    tdc::uliteral_t symbol_a = 'a';
    std::string code_should_be_a = "101";
    tdc::adaphuff::gen_code(temp, result, symbol_a, test_codingtree.root);
    std::string code_is = "";
    queue_to_string(result, code_is);
    ASSERT_TRUE(code_is.compare(code_should_be_a) == 0);

    temp = std::queue<bool>();
    result = std::queue<bool>();

    //Test for 'f'
    tdc::uliteral_t symbol_f = 'f';
    std::string code_should_be_f = "0001";
    tdc::adaphuff::gen_code(temp, result, symbol_f, test_codingtree.root);
    code_is = "";
    queue_to_string(result, code_is);
    ASSERT_TRUE(code_is.compare(code_should_be_f) == 0);

    delete_codingtree(&test_codingtree);
}

TEST (adaphuff, function_decode_get_char_from_code) {

}
