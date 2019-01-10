#pragma once

#include <vector>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {
namespace lz78 {

class BinarySortedTrie : public Algorithm, public LZ78Trie<> {
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
        Meta m(lz78_trie_type(), "binarysorted", "Lempel-Ziv 78 Sorted Binary Trie");
        return m;
    }
    inline BinarySortedTrie(Config&& cfg, size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
          , LZ78Trie(n,remaining_characters)
    {
        if(reserve > 0) {
            m_first_child.reserve(reserve);
            m_next_sibling.reserve(reserve);
            m_literal.reserve(reserve);
        }
    }

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

    inline node_t new_node(uliteral_t c, const factorid_t m_first_child_id, const factorid_t m_next_sibling_id) {
        m_first_child.push_back(m_first_child_id);
        m_next_sibling.push_back(m_next_sibling_id);
        m_literal.push_back(c);
        return node_t(size() - 1, true);
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

        DCHECK_LT(parent, size());


        if(m_first_child[parent] == undef_id) {
            m_first_child[parent] = newleaf_id;
            return new_node(c, undef_id, undef_id);
        } else {
            factorid_t node = m_first_child[parent];
            if(m_literal[node] > c) {
                m_first_child[parent] = newleaf_id;
                return new_node(c, undef_id, node);
            }
            while(true) { // search the binary tree stored in parent (following left/right siblings)
                if(c == m_literal[node]) return node_t(node, false);
                if(m_next_sibling[node] == undef_id) {
                    m_next_sibling[node] = newleaf_id;
                    return new_node(c, undef_id, undef_id);
                }
                const factorid_t nextnode = m_next_sibling[node];
                if(m_literal[nextnode] > c) {
                    m_next_sibling[node] = newleaf_id;
                    return new_node(c, undef_id, nextnode);
                }
                node = m_next_sibling[node];
                if(m_first_child.capacity() == m_first_child.size()) {
                    const size_t newbound =    m_first_child.size()+expected_number_of_remaining_elements(size());
                    if(newbound < m_first_child.size()*2 ) {
                        m_first_child.reserve   (newbound);
                        m_next_sibling.reserve  (newbound);
                        m_literal.reserve       (newbound);
                        IF_STATS(++m_specialresizes);
                    }
                    IF_STATS(++m_resizes);
                }
            }
        }
    }

    inline size_t size() const {
        return m_first_child.size();
    }

};

}} //ns

