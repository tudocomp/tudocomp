#pragma once

#include <tudocomp/config.h>
#ifdef JUDY_H_AVAILABLE

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz_trie/LZTrie.hpp>
#include <tudocomp/compressors/lz_trie/squeeze_node.hpp>
#include <Judy.h> // Judy Array

namespace tdc {
namespace lz_trie {

/// \brief LZ Trie Implementation
/// based on Julius Pettersson (MIT/Expat License.) and Juha Nieminen's work.
///
/// \sa http://www.cplusplus.com/articles/iL18T05o/
///
class JudyTrie : public Algorithm, public LZTrie<> {

    Pvoid_t m_dict; // judy array
    size_t m_size;

    inline factorid_t& find(const squeeze_node_t& node) {
        void* pvalue;
        JLI(pvalue, m_dict, node); //lookup node in dict, store value in pvalue
        DCHECK_NE(pvalue, PJERR);
        return *reinterpret_cast<factorid_t*>(pvalue);
    }

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "judy", "Lempel-Ziv 78 Judy Array");
        return m;
    }
    inline JudyTrie(Config&& cfg, size_t n, factorid_t = 0)
        : Algorithm(std::move(cfg))
        , LZTrie(n)
        , m_dict(static_cast<Pvoid_t>(nullptr))
        , m_size(0)
    {
    }

    inline node_t get_rootnode(uliteral_t c) const {
        DCHECK_LT(create_node(0,c), std::numeric_limits<factorid_t>::max());
        return node_t(create_node(0,c), false);
    }

    inline node_t add_rootnode(uliteral_t c) {
        DCHECK_EQ(find(create_node(0, c)), 0);
        find(create_node(0, c)) = size();
        ++m_size;
        return node_t(m_size-1, true);
    }

    inline void clear() {
        if(m_dict != nullptr) {
            int i;
            JLFA(i, m_dict);
        }
        m_size = 0;
    }

    inline node_t find_or_insert(const node_t& parent, uliteral_t c) {
        const squeeze_node_t node = create_node(parent.id(), c);

        factorid_t& id = find(node);

        if(id == 0) {
            const factorid_t newleaf_id = size()+1; //! if we add a new node, its index will be equal to the current size of the dictionary
            id = newleaf_id;
            ++m_size;
            return node_t(newleaf_id, true);
        }
        return node_t(id-1, false);
    }

    inline size_t size() const {
        return m_size;
    }
};

}} //ns

#endif// JUDY_H_AVAILABLE

