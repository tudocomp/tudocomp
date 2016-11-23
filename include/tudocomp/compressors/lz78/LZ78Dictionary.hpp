#ifndef LZ78_DICTIONARY_H
#define LZ78_DICTIONARY_H

///
/// @file
/// @author Julius Pettersson
/// @copyright MIT/Expat License.
/// @brief LZW file compressor
/// @version 6
/// @remarks This version borrows heavily from Juha Nieminen's work.
///
/// This is the C++11 implementation of a Lempel-Ziv-Welch single-file command-line compressor.
/// It was written with Doxygen comments.
///
/// It has been heavily adapted such that the dictionary search can be used
/// for both lz78 and lzw compressors
///
/// @see http://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch
/// @see http://marknelson.us/2011/11/08/lzw-revisited/
/// @see http://www.cs.duke.edu/csed/curious/compression/lzw.html
/// @see http://warp.povusers.org/EfficientLZW/index.html
/// @see http://en.cppreference.com/
/// @see http://www.doxygen.org/
///
///

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tdc {
namespace lz78 {

/// Type used to store and retrieve codes.
using factorid_t = std::uint32_t;
using CodeType = std::uint32_t; // TODO

/// Maximum legal dictionary size.
 const factorid_t DMS_MAX = std::numeric_limits<factorid_t>::max(); //TODO

///
/// @brief Encoder's custom dictionary type.
///
class EncoderDictionary {
    // struct Node;

	/*
	 * The trie is not stored in standard form. Each node stores the pointer to its first child (first as first come first served).
	 * The other children are stored in left_sibling/right_sibling of the first child (structured as a binary tree where the first child is the root, and the binary tree is sorted by the character of the trie edge)
	 */
	std::vector<factorid_t> first_child;
	std::vector<factorid_t> left_sibling;
	std::vector<factorid_t> right_sibling;
	std::vector<literal_t> literal;

    bool m_lzw_mode;

public:

    enum LzMode {
        Lz78,
        Lzw,
    };

    ///
    /// @brief Default constructor.
    /// @details It builds the `initials` cheat sheet.
    ///
    EncoderDictionary(LzMode mode, factorid_t dms, factorid_t reserve):
        m_lzw_mode(mode == Lzw)
    {
		DCHECK_EQ(dms, 0);
        first_child.reserve(reserve);
		left_sibling.reserve(reserve);
		right_sibling.reserve(reserve);
		literal.reserve(reserve);
        reset();
    }

    ///
    /// @brief Resets dictionary to its initial contents.
    ///
    void reset()
    {
        first_child.clear();
		left_sibling.clear();
		right_sibling.clear();
		literal.clear();

        // const long int minc = std::numeric_limits<uint8_t>::min();
        // const long int maxc = std::numeric_limits<uint8_t>::max();
        //
        if (m_lzw_mode) {
			first_child.resize(uliteral_max+1);
			left_sibling.resize(uliteral_max+1);
			right_sibling.resize(uliteral_max+1);
			literal.resize(uliteral_max+1);
			std::iota(literal.begin(),literal.end(),0);
		}
		else {
        first_child.push_back(0);
		left_sibling.push_back(0);
		right_sibling.push_back(0);
		literal.push_back(0);
		}
        // } else {
        //     // In lz78 mode the dictionary would start out empty,
        //     // but we need a root node for the intrinsic tree structure
        //     // to be able to actually add dictionary entries, so
        //     // add a dummy node.
        //     vn.push_back(Node('-', m_dms)); // dummy root node
        // }
    }

    ///
    /// @brief Searches for a pair (`i`, `c`) and inserts the pair if it wasn't found.
    /// @param i                code to search for
    /// @param c                attached byte to search for
    /// @return The index of the pair, if it was found.
    /// @retval m_dms    if the pair wasn't found
    ///
    factorid_t search_and_insert(const factorid_t& parent, uint8_t c)
    {
        // If we add a new node, its index will be equal to the current size
        // of the dictionary, so just keep it around beforehand.
        const factorid_t newleaf_id = position_to_index(first_child.size());


		if(m_lzw_mode && parent == 0) return c;
		

        // Starting at the end of the prefix string indicated by i,
        // walk the embedded linked list of child nodes
        // until there either is a match, or a empty place to insert:

		if(first_child[parent] == 0) {
			first_child[parent] = newleaf_id;
		} else {
        	factorid_t node = index_to_position(first_child[parent]);
            while(true) { // search the binary tree
                if(c < literal[node]) {
                    if (left_sibling[node] == 0) {
                        left_sibling[node] = newleaf_id;
                        break;
                    }
                    else
						node = index_to_position(left_sibling[node]);
                }
                else if (c > literal[node]) {
                    if (right_sibling[node] == 0) {
                        right_sibling[node] = newleaf_id;
                        break;
                    }
                    else
                        node = index_to_position(right_sibling[node]);
                }
                else /* c == literal[node] -> node is the node we want to find */ {
                    return position_to_index(node);
                }
            }
		}
        first_child.push_back(0);
		left_sibling.push_back(0);
		right_sibling.push_back(0);
		literal.push_back(c);
        return 0;
    }

	inline factorid_t index_to_position(const factorid_t& id) const {
		return id;
		// if(m_lzw_mode) return id;
		// DCHECK_GE(id,1);
		// return id-1;
	}

	inline factorid_t position_to_index(const factorid_t& id) const {
		return id;
		// if(m_lzw_mode) return id;
		// 	return id+1;
		}

    ///
    /// @brief Returns the number of dictionary entries.
    ///
    factorid_t size() const {
        return first_child.size();
    }
};

}} //ns

#endif
