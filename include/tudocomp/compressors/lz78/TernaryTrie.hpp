#pragma once

#include <vector>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {

/// \brief LZ78 Trie Implementation
/// based on Julius Pettersson (MIT/Expat License.) and Juha Nieminen's work.
///
/// \sa http://www.cplusplus.com/articles/iL18T05o/
///
class TernaryTrie : public Algorithm, public LZ78Trie<factorid_t> {

	/*
	 * The trie is not stored in standard form. Each node stores the pointer to its first child (first as first come first served).
	 * The other children are stored in left_sibling/right_sibling of the first child (structured as a binary tree where the first child is the root, and the binary tree is sorted by the character of the trie edge)
	 */
	std::vector<factorid_t> first_child;
	std::vector<factorid_t> left_sibling;
	std::vector<factorid_t> right_sibling;
	std::vector<uliteral_t> literal;

public:
    inline static Meta meta() {
        Meta m("lz78trie", "ternary", "Lempel-Ziv 78 Ternary Trie");
		return m;
	}
	
	/**
	 * @param remaining_characters number of remaining characters until the complete text is parsed
	 */
    TernaryTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0) 
		: Algorithm(std::move(env))
		, LZ78Trie(n, remaining_characters)
	{
		if(reserve > 0) {
			first_child.reserve(reserve);
			left_sibling.reserve(reserve);
			right_sibling.reserve(reserve);
			literal.reserve(reserve);
		}
    }

	IF_STATS(
		size_t m_resizes = 0;
		size_t m_specialresizes = 0;
		)

	IF_STATS(
		~TernaryTrie() {
		StatPhase::log("resizes", m_resizes);
		StatPhase::log("special resizes", m_specialresizes);
		StatPhase::log("table size", first_child.capacity());
		StatPhase::log("load ratio", first_child.size()*100/first_child.capacity());
		})

	node_t add_rootnode(uliteral_t c) override {
        first_child.push_back(undef_id);
		left_sibling.push_back(undef_id);
		right_sibling.push_back(undef_id);
		literal.push_back(c);
		return size() - 1;
	}

    node_t get_rootnode(uliteral_t c) override {
        return c;
    }

	void clear() override {
        first_child.clear();
		left_sibling.clear();
		right_sibling.clear();
		literal.clear();

	}

    node_t find_or_insert(const node_t& parent_w, uliteral_t c) override {
        auto parent = parent_w.id();

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
		// do not double the size if we only need fewer space
		if(first_child.capacity() == first_child.size()) {
			const size_t newbound =	first_child.size()+lz78_expected_number_of_remaining_elements(size(),m_n,m_remaining_characters);
			if(newbound < first_child.size()*2 ) {
				first_child.reserve   (newbound);
				left_sibling.reserve  (newbound);
				right_sibling.reserve (newbound);
				literal.reserve       (newbound);
				IF_STATS(++m_specialresizes);
			}
			IF_STATS(++m_resizes);
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

