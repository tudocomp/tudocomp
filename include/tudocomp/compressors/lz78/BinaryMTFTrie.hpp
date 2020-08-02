#pragma once

#include <vector>
#include <tudocomp/meta/TypeDesc.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {

class BinaryMTFTrie : public Algorithm, public LZ78Trie<> {

    /*
     * The trie is not stored in standard form. Each node stores the pointer to its first child and a pointer to its next sibling (first as first come first served)
     */
    std::vector<factorid_t> m_first_child;
    std::vector<factorid_t> m_next_sibling;
    std::vector<uliteral_t> m_literal;

    IF_STATS(
        size_t m_resizes = 0;
        size_t m_specialresizes = 0;
    )
public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "binaryMTF", "Lempel-Ziv 78 Binary Trie with Move-To-Front Heuristic");
        return m;
    }

    inline BinaryMTFTrie(Config&& cfg, size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
    : Algorithm(std::move(cfg))
    , LZ78Trie(n,remaining_characters)
    {
        if(reserve > 0) {
            m_first_child.reserve(reserve);
            m_next_sibling.reserve(reserve);
            m_literal.reserve(reserve);
        }
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~BinaryMTFTrie() {
            if (m_guard) {
                StatPhase::log("resizes", m_resizes);
                StatPhase::log("special resizes", m_specialresizes);
                StatPhase::log("table size", m_first_child.capacity());
                StatPhase::log("load ratio", m_first_child.size()*100/m_first_child.capacity());
            }
        }
    )
    BinaryMTFTrie(BinaryMTFTrie&& other) = default;
    BinaryMTFTrie& operator=(BinaryMTFTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        DCHECK_EQ(c, size());
        m_first_child.push_back(undef_id);
        m_next_sibling.push_back(undef_id);
        m_literal.push_back(c);
        return node_t(c, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
        m_first_child.clear();
        m_next_sibling.clear();
        m_literal.clear();
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

        DCHECK_LT(parent, size());


        if(m_first_child[parent] != undef_id) { //! if `parent` has already a child -> scan its children
            factorid_t node = m_first_child[parent]; //! node is the leftmost child of parent
			factorid_t prevnode = node;
            while(true) { //! iterate over all children of parent by selecting the next siblings of the current node `node` 
                if(c == m_literal[node]) {  //! found the child corresponding to (parent_w, c)
					if(prevnode != node) { //! node is not the leftmost child -> make it the leftmost child
						DCHECK_EQ(m_next_sibling[prevnode], node);
						m_next_sibling[prevnode] = m_next_sibling[node]; //! now the next sibling of prevnode is the next sibling of `node` (we omit `node`)
						m_next_sibling[node] = m_first_child[parent]; 
						m_first_child[parent] = node;
					}
					return node_t(node, false);
				}
                if(m_next_sibling[node] == undef_id) { //! there is no such child corresponding to (parent_w, c) -> create such a new child
                    break; //! we create this node below
                }
				prevnode = node;
                node = m_next_sibling[node];
            }
        }
        if(m_first_child.capacity() == m_first_child.size()) { //! if we reached the capacity, check whether we can use a heurstic to allocate less than double of the space
            const size_t newbound =    m_first_child.size()+expected_number_of_remaining_elements(size());
            if(newbound < m_first_child.size()*2 ) {
                m_first_child.reserve   (newbound);
                m_next_sibling.reserve  (newbound);
                m_literal.reserve       (newbound);
                IF_STATS(++m_specialresizes);
            }
            IF_STATS(++m_resizes);
        }
        m_first_child.push_back(undef_id);
        m_next_sibling.push_back(m_first_child[parent]); //! if parent does not have a child yet, then the new node does not get a sibling
        m_literal.push_back(c);
		m_first_child[parent] = newleaf_id;
		DCHECK_EQ(m_first_child.size()-1, newleaf_id);
        return node_t(size() - 1, true);
    }

    inline size_t size() const {
        return m_first_child.size();
    }
};

}} //ns

