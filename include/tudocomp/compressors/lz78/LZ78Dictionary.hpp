#ifndef LZ78_DICTIONARY_H
#define LZ78_DICTIONARY_H

/**
 * LZ78 Trie Implementation 
 * based on Julius Pettersson (MIT/Expat License.) and Juha Nieminen's work.
 * @see http://www.cplusplus.com/articles/iL18T05o/
**/

#include <limits>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <tudocomp/def.hpp>
#include <tudocomp/compressors/lz78/LZ78common.hpp>

namespace tdc {
namespace lz78 {

class EncoderDictionary {

	/*
	 * The trie is not stored in standard form. Each node stores the pointer to its first child (first as first come first served).
	 * The other children are stored in left_sibling/right_sibling of the first child (structured as a binary tree where the first child is the root, and the binary tree is sorted by the character of the trie edge)
	 */
	std::vector<factorid_t> first_child;
	std::vector<factorid_t> left_sibling;
	std::vector<factorid_t> right_sibling;
	std::vector<literal_t> literal;

public:
    EncoderDictionary(factorid_t reserve = 0) {
		if(reserve > 0) {
			first_child.reserve(reserve);
			left_sibling.reserve(reserve);
			right_sibling.reserve(reserve);
			literal.reserve(reserve);
		}
    }

	/**
	 * The dictionary can store multiple root nodes
	 * For LZ78, we use a root node with the id = 0.
	 * For LZW, we add for each possible literal value a root node.
	 * The compressor has to add these nodes.
	 */
	factorid_t add_rootnode(uliteral_t c) {
        first_child.push_back(undef_id);
		left_sibling.push_back(undef_id);
		right_sibling.push_back(undef_id);
		literal.push_back(c);
		return first_child.size();
	}
	/**
	 * Erases the contents of the dictionary.
	 * Used by compressors with limited dictionary size.
	 */
	void clear() {
        first_child.clear();
		left_sibling.clear();
		right_sibling.clear();
		literal.clear();

	}

    /** Searches a pair (`parent`, `c`). If there is no node below `parent` on an edge labeled with `c`, a new leaf of the `parent` will be constructed.
      * @param parent  the parent node's id
      * @param c       the edge label to follow
      * @return the index of the respective child, if it was found.
      * @retval undef_id   if a new leaf was inserted
      **/
    factorid_t find_or_insert(const factorid_t& parent, uint8_t c)
    {
        const factorid_t newleaf_id = first_child.size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		DCHECK_LT(parent, first_child.size());
		

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

	/**
	 * Returns the number of entries, plus the number of rootnodes
	 */
    factorid_t size() const {
        return first_child.size();
    }
};

}} //ns

#endif
