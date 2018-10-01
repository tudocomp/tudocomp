#pragma once

#include <limits>
#include <cstddef>
#include <cstdint>
#include <tudocomp/def.hpp>
namespace tdc {
namespace lz78 {

/// Type for the factor indices, bounded by the number of LZ78 trie nodes
using factorid_t = uint32_t;

/// Id that can be used for a non-existing factor
constexpr factorid_t undef_id = std::numeric_limits<factorid_t>::max();

/// Maximum legal dictionary size.
constexpr size_t DMS_MAX = std::numeric_limits<factorid_t>::max();

// NB: Also update the Lz78 chapter in the docs in case of changes to this file

/// Default return type of find_or_insert
class LZ78TrieNode {
    factorid_t m_id;
    bool m_is_new;
public:
    inline LZ78TrieNode(factorid_t id, bool is_new):
        m_id(id), m_is_new(is_new) {}
    inline LZ78TrieNode():
        LZ78TrieNode(0, false) {}

    inline bool is_new() const { return m_is_new; }
    inline factorid_t id() const { return m_id; }
};

#define LZ78_DICT_SIZE_DESC \
            "`dict_size` has to either be 0 (unlimited), or a positive integer,\n" \
            "and determines the maximum size of the backing storage of\n" \
            "the dictionary before it gets reset."

template<typename _node_t = LZ78TrieNode>
class LZ78Trie {
public:
    using node_t = _node_t;

    static inline constexpr TypeDesc lz78_trie_type() {
        return TypeDesc("lz78trie");
    }

private:
    const size_t m_n;
    const size_t& m_remaining_characters;
protected:
    LZ78Trie(const size_t n, const size_t& remaining_characters)
        : m_n(n), m_remaining_characters(remaining_characters) {}

    inline size_t expected_number_of_remaining_elements(const size_t z) const {
        return lz78_expected_number_of_remaining_elements(z, m_n, m_remaining_characters);
    }

    /**
     * The dictionary can store multiple root nodes
     * For LZ78, we use a root node with the id = c = 0.
     * For LZW, we add for each possible literal value a root node.
     * The compressor has to add these nodes.
     */
    inline node_t add_rootnode(uliteral_t c) {
        CHECK(false) << "This needs to be implemented by a inheriting class";
        return 0;
    }

    /**
     * Returns the root node corresponding to literal c
     */
    inline node_t get_rootnode(uliteral_t c) const {
        CHECK(false) << "This needs to be implemented by a inheriting class";
        return 0;
    }

    /**
     * Erases the contents of the dictionary.
     * Used by compressors with limited dictionary size.
     */
    inline void clear() {
        CHECK(false) << "This needs to be implemented by a inheriting class";
    }

    /** Searches a pair (`parent`, `c`). If there is no node below `parent` on an edge labeled with `c`, a new leaf of the `parent` will be constructed.
      * @param parent  the parent node's id
      * @param c       the edge label to follow
      * @return the new or found child node
      **/
    inline node_t find_or_insert(const node_t& parent, uliteral_t c) {
        CHECK(false) << "This needs to be implemented by a inheriting class";
        return 0;
    }

    /**
     * Returns the number of entries, plus the number of rootnodes
     */
    inline size_t size() const {
        CHECK(false) << "This needs to be implemented by a inheriting class";
        return 0;
    }

public:
    inline void debug_print() {}
};


}}//ns

