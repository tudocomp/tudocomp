#pragma once

#include <vector>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {

class BinaryTrie : public Algorithm, public LZ78Trie<factorid_t> {

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
        Meta m("lz78trie", "binary", "Lempel-Ziv 78 Binary Trie");
        return m;
    }

    inline BinaryTrie(Env&& env, size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
    : Algorithm(std::move(env))
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
        inline ~BinaryTrie() {
            if (m_guard) {
                StatPhase::log("resizes", m_resizes);
                StatPhase::log("special resizes", m_specialresizes);
                StatPhase::log("table size", m_first_child.capacity());
                StatPhase::log("load ratio", m_first_child.size()*100/m_first_child.capacity());
            }
        }
    )
    BinaryTrie(BinaryTrie&& other) = default;
    BinaryTrie& operator=(BinaryTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) override {
        m_first_child.push_back(undef_id);
        m_next_sibling.push_back(undef_id);
        m_literal.push_back(c);
        return size() - 1;
    }

    inline node_t get_rootnode(uliteral_t c) override {
        return c;
    }

    inline void clear() override {
        m_first_child.clear();
        m_next_sibling.clear();
        m_literal.clear();
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) override {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

        DCHECK_LT(parent, size());


        if(m_first_child[parent] == undef_id) {
            m_first_child[parent] = newleaf_id;
        } else {
            factorid_t node = m_first_child[parent];
            while(true) { // search the binary tree stored in parent (following left/right siblings)
                if(c == m_literal[node]) return node;
                if(m_next_sibling[node] == undef_id) {
                    m_next_sibling[node] = newleaf_id;
                    break;
                }
                node = m_next_sibling[node];
            }
        }
        if(m_first_child.capacity() == m_first_child.size()) {
            const size_t newbound =    m_first_child.size()+lz78_expected_number_of_remaining_elements(size(),m_n,m_remaining_characters);
            if(newbound < m_first_child.size()*2 ) {
                m_first_child.reserve   (newbound);
                m_next_sibling.reserve  (newbound);
                m_literal.reserve       (newbound);
                IF_STATS(++m_specialresizes);
            }
            IF_STATS(++m_resizes);
        }
        m_first_child.push_back(undef_id);
        m_next_sibling.push_back(undef_id);
        m_literal.push_back(c);
        return undef_id;
    }

    inline factorid_t size() const override {
        return m_first_child.size();
    }
};

}} //ns

