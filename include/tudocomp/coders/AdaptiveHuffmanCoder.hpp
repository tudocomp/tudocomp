#pragma once

#include <algorithm>
#include <bitset>
#include <numeric>
#include <queue>
#include <cstring>
#include <utility>
#include <cmath>
#include <string>
#include <limits>

#include <tudocomp/Env.hpp>
#include <tudocomp/Coder.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/def.hpp>


namespace tdc {

    namespace adaphuff {

#define SYMBOL 256
#define NUMBER 512


        struct node {
            uliteral_t symbol;
            unsigned int weight;
            unsigned int number;
            node *parent;
            node *left;
            node *right;

            inline bool is_leaf() {
                return (left == nullptr && right == nullptr);
            }

            inline bool has_left() {
                return left != nullptr;
            }

            inline bool has_right() {
                return right != nullptr;
            }

            inline bool has_children() {
                return has_left() && has_right();
            }

            inline char child_type() {
                //Error case
                if (this->parent == nullptr)
                    return 'f';
                //Left child
                if (this->parent->left->number == (this->number))
                    return 'l';
                //Right child
                if (this->parent->right->number == (this->number))
                    return 'r';
                //Fallback
                return 'f';
            }
        };

        struct codingtree {
            node *root;
            node *nyt;

            std::vector<bool> *dictionary;
        };

        /**
         * \brief Writes a node into a string
         * @param output_string
         * @param node
         */
        inline void node_to_string(std::string &output_string, node *node) {
            output_string += std::to_string(node->number);

            if (node->is_leaf()) {
                output_string += '[';
                if (node->weight == 0 && node->symbol == 0x00) {
                    output_string += "nyt; ";
                } else {
                    auto next_symbol = static_cast<char>(node->symbol);
                    output_string += "'";
                    output_string.append(1, next_symbol);
                    output_string += "' ";
                }
                auto next_weight = std::to_string(node->weight);
                output_string += next_weight;
                output_string += ']';
            } else {
                output_string += '(';
                auto next_weight = std::to_string(node->weight);
                output_string += next_weight;
                output_string += ')';
            }
        }

        /**
         * \brief Builds a string containing the tree structure level wise
         * @param root The root of the tree
         * @param output_string The string to write into
         */
        inline void tree_to_string(node *root, std::string &output_string) {
            std::queue<node *> queue_one, queue_two;
            std::queue<node *> *current_queue = &queue_one;
            std::queue<node *> *next_queue = &queue_two;

            next_queue->push(root);

            unsigned int level = 0;

            while (!next_queue->empty()) {
                std::string level_string = "Level " + std::to_string(level) + ':' + '\n';
                output_string.append(level_string);

                while (!current_queue->empty()) {
                    current_queue->pop();
                }
                std::queue<node *> *temp = current_queue;
                current_queue = next_queue;
                next_queue = temp;

                while (!current_queue->empty()) {
                    auto next_node_pointer = current_queue->front();

                    node_to_string(output_string, next_node_pointer);

                    current_queue->pop();
                    if (!next_node_pointer->is_leaf()) {
                        node *left_child = next_node_pointer->left;
                        node *right_child = next_node_pointer->right;
                        next_queue->push(left_child);
                        next_queue->push(right_child);
                    }
                }
                level++;
                output_string += '\n';
            }
        }

        typedef std::pair<int, node *> number_pair;

        inline bool order(number_pair p1, number_pair p2) {
            return p1.first < p2.first;
        }

        /**
         * \brief Creates a new node (for symbol or nyt)
         * @param leaf Adress of pointer that holds the new node
         * @param symbol The symbol that is represented by the node
         * @param is_nyt True if the new node is the nyt node
         */
        inline void create_node(node **leaf, uliteral_t symbol, bool is_nyt) {
            node *temp = new node;
            if (is_nyt) {
                temp->symbol = 0x00;
                temp->weight = 0;
            } else {
                temp->symbol = symbol;
                temp->weight = 0;
            }
            temp->parent = nullptr;
            temp->left = nullptr;
            temp->right = nullptr;

            *leaf = temp;
        }

        /**
         * \brief Creates an inner node for two nodes
         * @param parent Adresss of pointer to new node
         * @param left Left child of new node
         * @param right Right child of new node
         */
        inline void merge_node(node **parent, node *left, node *right) {
            node *temp = new node;
            temp->weight = left->weight + right->weight;
            temp->left = left;
            temp->right = right;
            temp->left->parent = temp;
            temp->right->parent = temp;
            temp->symbol = 0x00;
            temp->parent = nullptr;
            *parent = temp;
        }

        /**
         * \brief Finds the leaf with the corresponding symbol and returns it. If no such leaf exists nullptr
         * will be returned.
         * @param pTree The tree to search in
         * @param symbol The symbol to find
         * @return The node corresponding to the symbol
         */
        inline node *find_corresponding_leaf(node **pTree, uliteral_t symbol) {
            node *left_candidate = nullptr;
            node *right_candidate = nullptr;

            if(*pTree == nullptr)
                return nullptr;

            if ((*pTree)->has_left()) {
                left_candidate = find_corresponding_leaf(&(*pTree)->left, symbol);
            }

            if ((*pTree)->symbol == symbol && (*pTree)->is_leaf() && (*pTree)->weight != 0) {
                return *pTree;
            }

            if ((*pTree)->has_right()) {
                right_candidate = find_corresponding_leaf(&(*pTree)->right, symbol);
            }

            if (left_candidate != nullptr) {
                return left_candidate;
            } else if (right_candidate != nullptr) {
                return right_candidate;
            } else {
                return nullptr;
            }
        }

        /**
         * \brief Finds node of specific number
         * @param pTree
         * @param number
         * @param result
         */
        inline void find_node(node *pTree, unsigned int const number, node*& result) {
            if (pTree->number == number) {
                result = pTree;
                return;
            }
            if (pTree->has_children() && pTree->number >= number) {
                find_node(pTree->left, number, result);
                find_node(pTree->right, number, result);
            }
        }

        /**
         * \brief Deletes the tree
         * @param pTree
         */
        inline void delete_tree(node **pTree) {
            if ((*pTree) == nullptr)
                return;

            if ((*pTree)->has_left()) {
                delete_tree(&(*pTree)->left);
            }

            if ((*pTree)->has_right()) {
                delete_tree(&(*pTree)->right);
            }

            if ((*pTree) != nullptr) {
                if ((*pTree)->parent != nullptr && (*pTree)->left == nullptr) {
                    (*pTree)->parent->left = nullptr;
                    delete *pTree;

                } else if ((*pTree)->parent != nullptr && (*pTree)->right == nullptr) {
                    (*pTree)->parent->right = nullptr;
                    delete *pTree;

                } else if ((*pTree)->right == nullptr && (*pTree)->left == nullptr) {
                    delete *pTree;
                    (*pTree) = nullptr;
                }
            }
        }

        /**
         * \brief Increments the weight of the given node.
         * For internal nodes adds weight of left and right childeren
         * For leafs increases the weight by one
         * @param pNode The node to increment its weight
         */
        inline void increment_weight(node **pNode) {
            (*pNode)->weight++;
        }


        /**
         * \brief Find for a given node the block leader of nodes of \p weight
         * @param tree Root of current tree to search in
         * @param weight Weight of block to search for
         * @param number Currently highest number found
         * @param parent_number Number of parent node
         * @param position Block-leader
         */
        inline void search_higher_block(node **tree, const unsigned int weight, unsigned int *number, const unsigned int parent_number, node **position) {
            if ((*tree)->weight == weight && (*tree)->number > *number && (*tree)->number != parent_number) {
                *position = (*tree);
                *number = (*tree)->number;
            }

            if ((*tree)->has_left()) {
                search_higher_block(&(*tree)->left, weight, &*number, parent_number, &*position);
            }

            if ((*tree)->has_right()) {
                search_higher_block(&(*tree)->right, weight, &*number, parent_number, &*position);
            }
        }



        /**
         * Switches the \p l_r child of \p tree and \p l_r_sibilng child of \p sibling
         * @param tree
         * @param l_r
         * @param sibling
         * @param l_r_sibling
         */
        inline void switch_node(node *node_one, char l_r_one, node *node_two, char l_r_two) {

            if (l_r_one == 'l' && l_r_two == 'l') {
                node *temp = node_one->left;
                node_one->left = node_two->left;
                node_two->left = temp;

                node_one->left->parent = node_one;
                node_two->left->parent = node_two;
            } else if (l_r_one == 'l' && l_r_two == 'r') {
                node *temp = node_one->left;
                node_one->left = node_two->right;
                node_two->right = temp;

                node_one->left->parent = node_one;
                node_two->right->parent = node_two;
            } else if (l_r_two == 'l') {
                node *temp = node_one->right;
                node_one->right = node_two->left;
                node_two->left = temp;

                node_one->right->parent = node_one;
                node_two->left->parent = node_two;
            } else {
                node *temp = node_one->right;
                node_one->right = node_two->right;
                node_two->right = temp;

                node_one->right->parent = node_one;
                node_two->right->parent = node_two;
            }
        }

        /**
         * \brief Switches the position of two nodes
         * \warning Does not work for switching children with ancestors
         * @param node_one
         * @param node_two
         */
        inline void switch_nodes(node* node_one, node* node_two) {
            char l_r_one = node_one->child_type();
            char l_r_two = node_two->child_type();
            switch_node(node_one->parent, l_r_one, node_two->parent, l_r_two);
        }


        /**
         * TODO Chris
         * @param queue
         * @param pTree
         * @param depth
         */
        void queue_nodes(std::vector<number_pair> *queue, node **pTree, int depth) {
            queue->push_back(std::make_pair(depth, *pTree));

            if ((*pTree)->has_right()) {
                queue_nodes(&*queue, &(*pTree)->right, depth + 1);
            }

            if ((*pTree)->has_left()) {
                queue_nodes(&*queue, &(*pTree)->left, depth + 1);
            }
        }

        inline bool check_internal_slide(node *p_node, unsigned int p_weight) {
            bool next_internal_same_weight = !p_node->is_leaf() && p_node->weight == p_weight;
            bool next_leaf_one_heavier = p_node->is_leaf() && p_node->weight == p_weight+1;

            return  next_internal_same_weight || next_leaf_one_heavier;
        }

        inline bool check_leaf_slide(node *p_node, unsigned int p_weight) {
            //Slide ahead when next is same weight
            return p_node->weight == p_weight;
        }


        inline void slide_ahead(node **p_tree, node **p_node, bool (*condition)(node*, unsigned int)) {
            unsigned int weight = (*p_node)->weight;
            unsigned int number = ((*p_node)->number)+1;
            node* next_node = nullptr;
            find_node(*p_tree, number, next_node);

            //Slide p ahead
            while (next_node != nullptr && condition(next_node,weight)) {
                switch_nodes((*p_node), next_node);
                number++;
                find_node(*p_tree, number, next_node);
            }
        }

        /**
         * \brief Slides a node to the correct position.
         * Root node will not be slided.
         * @param p_tree The root node of the tree
         * @param p_node The node which needs to be slided
         */
        inline void slide_and_increment(node **p_tree, node** p_node) {
            //Roots do not slide!
            if(*p_tree == *p_node) {
                increment_weight(p_node);
                *p_node = nullptr;
                return;
            }

            //previous_p = parent of p
            node* previous_p = (*p_node)->parent;

            if ((*p_node)->is_leaf()) {
                //Slide leaf node

                bool (*condition)(node*, unsigned int) = &check_leaf_slide;
                slide_ahead(p_tree, p_node, condition);
                // increase weight of p by 1
                increment_weight(p_node);
                // p = new parent of p
                *p_node = (*p_node)->parent;
            } else {
                //Slide internal node

                bool (*condition)(node*, unsigned int) = &check_internal_slide;
                slide_ahead(p_tree, p_node, condition);
                // increase weight of p by 1
                increment_weight(p_node);
                //p = previous_p
                *p_node = previous_p;
            }
        }

        /**
         * \brief Assigns the implicit numbering to the nodes of the tree.
         * @param queue The queue which contains the nodes in reverse order of the numbering
         * @param codingtree The codingtree holding the tree
         */
        inline void implicit_numbering(codingtree *codingtree) {
            std::vector<number_pair> queue;

            // queue and sort nodes
            queue_nodes(&queue, &codingtree->root, 0);
            std::sort(queue.begin(), queue.end(), order);

            // assign implicit numbering to the nodes
            int num = NUMBER;
            for (unsigned int i = 0; i < queue.size(); i++) {
                queue.at(i).second->number = num--;
            }

            queue.clear();
        }

        inline void print_dict(std::vector<bool> *dict) {
            std::string output = "";
            for (unsigned int i =0; i < SYMBOL; i++) {
                std::string var = to_str(i);
                output.append("(" + var + ": ");
                if (dict->at(i)) {
                    output.append("1 " + to_str(dict->at(i)) + ")");
                } else {
                    output.append("0 " + to_str(dict->at(i)) + ")");
                }
            }

            std::cout << output << std::endl;
        }

        inline bool dict_touched(std::vector<bool> *dict) {
            for (unsigned int i =0; i < SYMBOL; i++) {
                if (dict->at(i)) {
                    return true;
                }
            }
            return false;
        }

        /**
         * \brief TODO
         * @param codingtree The codingtree holding the structure
         * @param symbol The symbol to update
         */
        inline void update(codingtree *codingtree, uliteral_t symbol) {
//          leaf ToIncrement := 0;
            node *leafToIncrement = nullptr;
//          q := leaf node corresponding to a_{i_{t+1}};
            node *correspondingLeaf = find_corresponding_leaf(&codingtree->root, symbol);

//          if (q is the O-node (aka nyt)) and (k < n - 1) then
            std::vector<bool>* dict = codingtree->dictionary;

            unsigned int position = static_cast<unsigned int>(symbol);
            bool leaf_exists = dict->at(position);
            if (!leaf_exists) {
                // create new NYT node
                node *new_nyt;
                create_node(&new_nyt, 0x00, true);

                // create new symbol node
                node *symbol_node;
                create_node(&symbol_node, symbol, false);

                // Replace q by an internal O-node with two leaf O-node children,
                // such that the right child corresponds to a_{i_{t+1}}
                node *old_nyt = nullptr;
                merge_node(&old_nyt, new_nyt, symbol_node);

                if (codingtree->root == nullptr) {
                    codingtree->root = old_nyt;
                    codingtree->nyt = codingtree->root->left;
                } else {
                    node *nyt = codingtree->nyt;
                    old_nyt->parent = codingtree->nyt->parent;
                    codingtree->nyt->parent->left = old_nyt;
                    codingtree->nyt = old_nyt->left;
                    delete nyt;
                }

                implicit_numbering(codingtree);

                // q := internal 0-node just created
                correspondingLeaf = old_nyt;

                // leaf ToIncrement := the right child of q
                leafToIncrement = correspondingLeaf->right;

                dict->at(position) = true;
            } else {
                // Corresponding leaf does exist
                node *inner_node = nullptr;

                unsigned int number = correspondingLeaf->number;
                search_higher_block(&codingtree->root, correspondingLeaf->weight, &number,
                                    correspondingLeaf->parent->number, &inner_node);
//                  Interchange q in the tree with the leader of its block;
//                  if q is the sibling of the O-node then
                if (inner_node != nullptr) {
                    switch_nodes(correspondingLeaf, inner_node);
                }
                // leaf ToIncrement := q;
                //leafToIncrement = correspondingLeaf;
                // q := parent of q
                //correspondingLeaf = correspondingLeaf->parent;
            }

//              while q is not the root of the Huffman tree do
//              (Main loop; q must he the leader of its block]
//                  SlideAndZncrement(q);
//              if leaf Tolncrement # 0 then (Handle the two special cases)
//                  SlideAndIncrement(leaf ToIncrement)
            while (correspondingLeaf != nullptr) {
                slide_and_increment(&codingtree->root, &correspondingLeaf);

                implicit_numbering(codingtree);
            }
            if (leafToIncrement != nullptr) {
                slide_and_increment(&codingtree->root, &leafToIncrement);

                implicit_numbering(codingtree);
            }

        }



        /**
         * Encode
         */

        /**
         * TODO
         * \brief Writes the code for a given symbol into the queue code_write
         * @param do_code
         * @param code_write
         * @param symbol
         * @param pTree
         */
        inline void
        gen_code(std::queue<bool> &do_code, std::queue<bool> &code_write, const uliteral_t symbol, node *pTree) {

            // symbol found?
            if ((pTree)->symbol == symbol && (pTree)->is_leaf() && (pTree)->weight != 0) {
                while (!do_code.empty()) {
                    bool next_bit = do_code.front();
                    do_code.pop();
                    code_write.push(next_bit);
                }
                return;
            }

            std::queue<bool> temp_left(do_code);
            if ((pTree)->has_left()) {
                temp_left.push(false);
                gen_code(temp_left, code_write, symbol, (pTree)->left);
            }

            std::queue<bool> temp_right(do_code);
            if ((pTree)->has_right()) {
                temp_right.push(true);
                gen_code(temp_right, code_write, symbol, (pTree)->right);
            }

            return;
        }

        /**
         * \brief Creates the nyt-code for the current tree
         * @param do_code Temporary queue
         * @param write_code Result queue
         * @param pTree Current node
         */
        inline void gen_nyt_code(std::queue<bool> &do_code, std::queue<bool> &write_code, node *pTree) {

            if ((pTree)->weight == 0 && (pTree)->is_leaf()) {
                while (!do_code.empty()) {
                    bool next_bit = do_code.front();
                    do_code.pop();
                    write_code.push(next_bit);
                }
            }

            std::queue<bool> temp_left(do_code);
            if ((pTree)->has_left()) {
                temp_left.push(false);
                gen_nyt_code(temp_left, write_code, (pTree)->left);
            }

            std::queue<bool> temp_right(do_code);
            if ((pTree)->has_right()) {
                temp_right.push(true);
                gen_nyt_code(temp_right, write_code, (pTree)->right);
            }

        }

        /**
         * Creates the standart encoding for a symbol
         * @param code
         * @param symbol
         */
        inline void gen_standard_code(std::queue<bool> &code, uliteral_t symbol) {
            unsigned int symbol_size = sizeof(uliteral_t)*CHAR_BIT;
            unsigned int bit_mask = pow(2, symbol_size - 1);

            for (unsigned int i = 0; i < symbol_size; i++) {
                if ((symbol & bit_mask) == bit_mask) {
                    code.push(true);
                } else {
                    code.push(false);
                }
                symbol <<= 1;
            }
        }


        inline void encode(codingtree *enc_codingtree, tdc::io::BitOStream &os, uliteral_t symbol) {
            node *tree = enc_codingtree->root;
            std::vector<bool> *dictionary = enc_codingtree->dictionary;

            std::queue<bool> do_code, final_code;



            // if node representing symbol exists
            if ((*dictionary)[static_cast<unsigned int>(symbol)]) {
                gen_code(do_code, final_code, symbol, tree);
            } else {
                // Node representing symbol does not exist
                if (tree != nullptr) {
                    gen_nyt_code(do_code, final_code, tree);
                }
                gen_standard_code(final_code, symbol);
            }

            bool sign;
            while (!final_code.empty()) {
                sign = final_code.front();
                final_code.pop();
                os.write_bit(sign);
            }

            update(enc_codingtree, symbol);
        }

        /**
         * Decode
         */

        // Pulls correct number of bits form input stream
        inline uliteral_t get_char_from_code(tdc::io::BitIStream &is) {
            uliteral_t temp;
            temp &= 0x00;

            unsigned int symbol_size = sizeof(uliteral_t)*CHAR_BIT;
            for (unsigned int i = 0; i < symbol_size; i++) {
                if (is.read_bit() == 1) {
                    temp ^= 0x01;
                }
                if (i != 7) {
                    temp <<= 1;
                }
            }

            return temp;

        }


        /**
         * \brief Decodes the next character from the input stream
         * @param is
         * @param dec_codingtree
         * @return
         */
        inline uliteral_t decode(
                tdc::io::BitIStream &is,
                codingtree *dec_codingtree) {


            node *temp = dec_codingtree->root;

            std::queue<char> code_read;

            // The tree is empty
            if (temp == nullptr)
            {
                uliteral_t symbol = get_char_from_code(is);
                update(dec_codingtree, symbol);
                return symbol;
            } else
                // The tree exists, we need to walk it down until we reach a leaf
            {
                while (!temp->is_leaf()) {
                    if (is.read_bit() == 0) {
                        temp = temp->left;
                        code_read.push('0');
                    } else {
                        temp = temp->right;
                        code_read.push('1');
                    }
                }

                if (temp->weight == 0)
                    // The nyt-node is found
                {
                    uliteral_t symbol = get_char_from_code(is);
                    update(dec_codingtree, symbol);
                    return symbol;
                } else
                    // The corresponding node was found
                {
                    update(dec_codingtree, temp->symbol);
                    return temp->symbol;
                }
            }
        }

    }//ns


    class AdaptiveHuffmanCoder : public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("coder", "adaphuff", "Adaptive Huffman Coder");
            return m;
        }

        inline static void initialize_codingtree(adaphuff::codingtree &codingtree) {
            codingtree.root = nullptr;
            codingtree.nyt = nullptr;
            std::vector<bool> *dictionary = new std::vector<bool>(SYMBOL, false);
            codingtree.dictionary = dictionary;
        }

        inline static void delete_codingtree(adaphuff::codingtree &codingtree) {
            delete codingtree.dictionary;
            delete_tree(&(codingtree.root));
        }

        AdaptiveHuffmanCoder() = delete;

        class Encoder : public tdc::Encoder {
            adaphuff::codingtree enc_codingtree;

        public:
            // Constructor 1
            template<typename literals_t>
            inline Encoder(Env &&env, std::shared_ptr<BitOStream> out, literals_t &&literals)
                    : tdc::Encoder(std::move(env), out, literals) {
                initialize_codingtree(enc_codingtree);
            }

            // Constructor 2
            template<typename literals_t>
            inline Encoder(Env &&env, Output &out, literals_t &&literals)
                    : Encoder(std::move(env), std::make_shared<BitOStream>(out), literals) {
            }

            // Destructor
            ~Encoder() {
                delete_codingtree(enc_codingtree);
            }

            using tdc::Encoder::encode; // default encoding as fallback

            /// \brief Encodes a single literal
            template<typename value_t>
            inline void encode(value_t v, const LiteralRange &) {
                adaphuff::encode(&enc_codingtree, *m_out, v);
            }

        };

        class Decoder : public tdc::Decoder {
            adaphuff::codingtree dec_codingtree;
        public:
            // Constructor 1
            inline Decoder(Env &&env, std::shared_ptr<BitIStream> in)
                    : tdc::Decoder(std::move(env), in) {
                initialize_codingtree(dec_codingtree);
            }

            // Constructor 2
            inline Decoder(Env &&env, Input &in)
                    : Decoder(std::move(env), std::make_shared<BitIStream>(in)) {
            }

            ~Decoder() {
                delete_codingtree(dec_codingtree);
            }

            using tdc::Decoder::decode; // default decoding as fallback

            template<typename value_t>
            inline value_t decode(const LiteralRange &) {
                return adaphuff::decode(*m_in, &dec_codingtree);
            }
        };
    };


}//ns