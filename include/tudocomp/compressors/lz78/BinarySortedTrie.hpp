#ifndef BINARYSORTEDTRIE_HPP
#define BINARYSORTEDTRIE_HPP


#include <vector>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {
namespace lz78 {

class BinarySortedTrie : public Algorithm, public LZ78Trie<factorid_t> {
	/*
	 * The trie is not stored in standard form. Each node stores the pointer to its first child and a pointer to its next sibling (first as first come first served)
	 */
	std::vector<factorid_t> first_child;
	std::vector<factorid_t> next_sibling;
	std::vector<literal_t> literal;

public:
    using search_pos_t = factorid_t;

    inline static Meta meta() {
        Meta m("lz78trie", "binarysorted", "Lempel-Ziv 78 Sorted Binary Trie");
		return m;
	}
    BinarySortedTrie(Env&& env, factorid_t reserve = 0) : Algorithm(std::move(env)) {
		if(reserve > 0) {
			first_child.reserve(reserve);
			next_sibling.reserve(reserve);
			literal.reserve(reserve);
		}
    }

	node_t add_rootnode(uliteral_t c) override {
        first_child.push_back(undef_id);
		next_sibling.push_back(undef_id);
		literal.push_back(c);
		return size() - 1;
	}

    node_t get_rootnode(uliteral_t c) override {
        return c;
    }

	void clear() override {
        first_child.clear();
		next_sibling.clear();
		literal.clear();

	}

	inline factorid_t new_node(uliteral_t c, const factorid_t first_child_id, const factorid_t next_sibling_id) {
        first_child.push_back(first_child_id);
		next_sibling.push_back(next_sibling_id);
		literal.push_back(c);
		return undef_id;
	}

    node_t find_or_insert(const node_t& parent_w, uliteral_t c) override {
        auto parent = parent_w.factorid();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		DCHECK_LT(parent, size());


		if(first_child[parent] == undef_id) {
			first_child[parent] = newleaf_id;
			return new_node(c, undef_id, undef_id);
		} else {
        	factorid_t node = first_child[parent];
			if(literal[node] > c) {
				first_child[parent] = newleaf_id;
				return new_node(c, undef_id, node);
			}
            while(true) { // search the binary tree stored in parent (following left/right siblings)
				if(c == literal[node]) return node;
				if(next_sibling[node] == undef_id) {
					next_sibling[node] = newleaf_id;
					return new_node(c, undef_id, undef_id);
				}
				const factorid_t nextnode = next_sibling[node];
				if(literal[nextnode] > c) {
					next_sibling[node] = newleaf_id;
					return new_node(c, undef_id, nextnode);
				}
				node = next_sibling[node];
            }
		}
		DCHECK(false);
        return undef_id;
    }

    factorid_t size() const override {
        return first_child.size();
    }
};

}} //ns

#endif /* BINARYSORTEDTRIE_HPP */
