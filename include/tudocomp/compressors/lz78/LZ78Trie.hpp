#ifndef LZ78TRIE_HPP
#define LZ78TRIE_HPP

#include <limits>
#include <cstddef>
#include <cstdint>
#include <tudocomp/def.hpp>
namespace tdc {
namespace lz78 {

using factorid_t = std::uint32_t; //! type for the factor indices, bounded by the number of LZ78 trie nodes
static constexpr factorid_t undef_id = std::numeric_limits<factorid_t>::max(); // for a non-existing factor

/// Maximum legal dictionary size.
const factorid_t DMS_MAX = std::numeric_limits<factorid_t>::max(); //TODO

/// Return type of find_or_insert
template<typename search_pos_t>
class TrieNode {
};

/// Return type of find_or_insert
template<>
class TrieNode<factorid_t> {
    factorid_t m_factorid_and_search_pos;
public:
    TrieNode(const factorid_t& factorid_and_search_pos):
        m_factorid_and_search_pos(factorid_and_search_pos){}
    TrieNode(): TrieNode(0) {}

    inline const factorid_t& factorid() const { return m_factorid_and_search_pos; }
    inline factorid_t& factorid() { return m_factorid_and_search_pos; }
    inline const factorid_t& search_pos() const { return m_factorid_and_search_pos; }
    inline bool is_factorid_undefined() { return false; }
};

#define LZ78_DICT_SIZE_DESC \
			"`dict_size` has to either be 0 (unlimited), or a positive integer,\n" \
			"and determines the maximum size of the backing storage of\n" \
			"the dictionary before it gets reset."

template<typename search_pos>
class LZ78Trie {
public:
    using search_pos_t = search_pos;
    using node_t = TrieNode<search_pos_t>;
protected:

	/**
	 * The dictionary can store multiple root nodes
	 * For LZ78, we use a root node with the id = c = 0.
	 * For LZW, we add for each possible literal value a root node.
	 * The compressor has to add these nodes.
	 */
	virtual node_t add_rootnode(uliteral_t c) = 0;

    /**
     * Returns the root node corresponding to literal c
     */
    virtual node_t get_rootnode(uliteral_t c) = 0;

	/**
	 * Erases the contents of the dictionary.
	 * Used by compressors with limited dictionary size.
	 */
	virtual void clear() = 0;

    /** Searches a pair (`parent`, `c`). If there is no node below `parent` on an edge labeled with `c`, a new leaf of the `parent` will be constructed.
      * @param parent  the parent node's id
      * @param c       the edge label to follow
      * @return the index of the respective child, if it was found.
      * @retval undef_id   if a new leaf was inserted
      **/
    virtual node_t find_or_insert(const node_t& parent, uliteral_t c) = 0;

	/**
	 * Returns the number of entries, plus the number of rootnodes
	 */
    virtual factorid_t size() const = 0;

};


}}//ns

#endif /* LZ78TRIE_HPP */
