#ifndef TERNARYTRIE_HPP
#define TERNARYTRIE_HPP

/**
 * LZ78 Trie Implementation 
 * based on Julius Pettersson (MIT/Expat License.) and Juha Nieminen's work.
 * @see http://www.cplusplus.com/articles/iL18T05o/
**/

#include <vector>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {
namespace lz78 {

class TernaryTrie : public Algorithm, public LZ78Trie {

	/*
	 * The trie is not stored in standard form. Each node stores the pointer to its first child (first as first come first served).
	 * The other children are stored in left_sibling/right_sibling of the first child (structured as a binary tree where the first child is the root, and the binary tree is sorted by the character of the trie edge)
	 */
	std::vector<factorid_t> first_child;
	std::vector<factorid_t> left_sibling;
	std::vector<factorid_t> right_sibling;
	std::vector<literal_t> literal;

public:
    inline static Meta meta() {
        Meta m("lz78trie", "ternary", "Lempel-Ziv 78 Ternary Trie");
		return m;
	}
    TernaryTrie(Env&& env, factorid_t reserve = 0) : Algorithm(std::move(env)) {
		if(reserve > 0) {
			first_child.reserve(reserve);
			left_sibling.reserve(reserve);
			right_sibling.reserve(reserve);
			literal.reserve(reserve);
		}
    }

	factorid_t add_rootnode(uliteral_t c) override {
        first_child.push_back(undef_id);
		left_sibling.push_back(undef_id);
		right_sibling.push_back(undef_id);
		literal.push_back(c);
		return size();
	}

	void clear() override {
        first_child.clear();
		left_sibling.clear();
		right_sibling.clear();
		literal.clear();

	}

    factorid_t find_or_insert(const factorid_t& parent, uliteral_t c) override {
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		DCHECK_LT(parent, size());
		

		if(first_child[parent] == undef_id) {
			first_child[parent] = newleaf_id;
		} else {
        	factorid_t node = first_child[parent];
            while(true) { // search the binary tree stored in parent (following left/right siblings)
                if(c < literal[node]) {
                    if (left_sibling[node] == undef_id) {
                        left_sibling[node] = newleaf_id;
                        break;
                    }
                    else
						node = left_sibling[node];
                }
                else if (c > literal[node]) {
                    if (right_sibling[node] == undef_id) {
                        right_sibling[node] = newleaf_id;
                        break;
                    }
                    else
                        node = right_sibling[node];
                }
                else /* c == literal[node] -> node is the node we want to find */ {
                    return node;
                }
            }
		}
        first_child.push_back(undef_id);
		left_sibling.push_back(undef_id);
		right_sibling.push_back(undef_id);
		literal.push_back(c);
        return undef_id;
    }

    factorid_t size() const override {
        return first_child.size();
    }
};

}} //ns

#endif /* TERNARYTRIE_HPP */
