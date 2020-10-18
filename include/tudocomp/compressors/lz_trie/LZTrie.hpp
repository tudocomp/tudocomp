#pragma once

#include <limits>
#include <cstddef>
#include <cstdint>
#include <tudocomp/def.hpp>
#include <memory>

#include <tudocomp/compressors/lz_common/factorid_t.hpp>

namespace tdc {namespace lz_trie {
using lz_common::factorid_t;
using lz_common::undef_id;

class SharedRemainingElementsHint {
    struct Inner { size_t m_n; size_t m_remaining_characters; };
    std::shared_ptr<Inner> m_ptr;
public:
    inline SharedRemainingElementsHint() = default;

    inline SharedRemainingElementsHint(size_t initial_size):
        m_ptr(std::make_shared<Inner>(Inner { initial_size, initial_size })) {}

    inline size_t size() const { return m_ptr->m_n; }
    inline size_t remaining_characters() const { return m_ptr->m_remaining_characters; }
    inline size_t& remaining_characters() { return m_ptr->m_remaining_characters; }

    inline size_t expected_number_of_remaining_elements(const size_t z) const {
        return lz78_expected_number_of_remaining_elements(z, size(), remaining_characters());
    }
};

// NB: Also update the Lz78 chapter in the docs in case of changes to this file

/// Default return type of find_or_insert
class LZTrieNode {
    factorid_t m_id;
    bool m_is_new;
public:
    inline LZTrieNode(factorid_t id, bool is_new):
        m_id(id), m_is_new(is_new) {}
    inline LZTrieNode():
        LZTrieNode(0, false) {}

    inline bool is_new() const { return m_is_new; }
    inline factorid_t id() const { return m_id; }
};

#define LZ78_DICT_SIZE_DESC \
            "`dict_size` has to either be 0 (unlimited), or a positive integer,\n" \
            "and determines the maximum size of the backing storage of\n" \
            "the dictionary before it gets reset."

template<typename _node_t = LZTrieNode>
class LZTrie {
public:
    using node_t = _node_t;

    static inline constexpr TypeDesc lz_trie_type() {
        return TypeDesc("lz_trie");
    }

private:
    SharedRemainingElementsHint m_remaining_hint;
protected:
    LZTrie(SharedRemainingElementsHint hint): m_remaining_hint(hint) {}

    inline SharedRemainingElementsHint remaining_elements_hint() const { return m_remaining_hint; }
    inline size_t expected_number_of_remaining_elements(const size_t z) const {
        return m_remaining_hint.expected_number_of_remaining_elements(z);
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

    /*
    template<typename Self, typename node_callback>
    inline static node_t find_or_insert_many(Self& self, std::istream& inp, node_t& rootnode, size_t& remaining_characters) {
        char c;
        while(inp.get(c)) {
            --remaining_characters;
            node_t child = self.find_or_insert(node, static_cast<uliteral_t>(c));
            if(child.is_new()) {
                lz78::encode_factor(coder, node.id(), static_cast<uliteral_t>(c), factor_count);
                factor_count++;
                IF_STATS(stat_factor_count++);
                parent = node = dict.get_rootnode(0); // return to the root
                DCHECK_EQ(node.id(), 0);
                DCHECK_EQ(parent.id(), 0);
                DCHECK_EQ(factor_count+1, dict.size());
                // dictionary's maximum size was reached
                if(tdc_unlikely(dict.size() == m_dict_max_size)) { // if m_dict_max_size == 0 this will never happen
                    DCHECK(false); // broken right now
                    reset_dict();
                    factor_count = 0; //coder.dictionary_reset();
                    IF_STATS(stat_dictionary_resets++);
                    IF_STATS(stat_dict_counter_at_last_reset = m_dict_max_size);
                }
            } else { // traverse further
                parent = node;
                node = child;
            }
        }

        return rootnode;
    }
    */

public:
    inline void debug_print() {}

    /// Indicate that we just consumed a byte of input and are about to
    /// potentially insert a new element into thr trie.
    inline void signal_character_read() {
        DCHECK(m_remaining_hint.remaining_characters() != 0);
        --m_remaining_hint.remaining_characters();
    }
};


}}//ns

