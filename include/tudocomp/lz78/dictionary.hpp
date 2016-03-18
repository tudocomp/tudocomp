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
/// It has been changed such that the dictionary search can be used
/// for both lz78 and lzw compressors
///
/// @see http://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch
/// @see http://marknelson.us/2011/11/08/lzw-revisited/
/// @see http://www.cs.duke.edu/csed/curious/compression/lzw.html
/// @see http://warp.povusers.org/EfficientLZW/index.html
/// @see http://en.cppreference.com/
/// @see http://www.doxygen.org/
///
/// @remarks DF: the data file
/// @remarks EF: the encoded file
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

namespace lz78_dictionary {

// Safety macro; if not defined, some overkillish safety checks are avoided.
//#define TAKE_NO_RISKS

/// Type used to store and retrieve codes.
using CodeType = std::uint32_t;

/*namespace globals {

/// Dictionary Maximum Size (when reached, the dictionary will be reset)
//const CodeType dms {512 * 1024};
const CodeType dms {std::numeric_limits<CodeType>::max()};
const CodeType reserve_dms {0};

}*/ // namespace globals

///
/// @brief Special codes used by the encoder to control the decoder.
/// @todo Metacodes should not be hardcoded to match their index.
///
enum class MetaCode: CodeType {
    Eof = 1u << CHAR_BIT,   ///< End-of-file.
};

///
/// @brief Encoder's custom dictionary type.
///
class EncoderDictionary {

    ///
    /// @brief Binary search tree node.
    ///
    struct Node {

        ///
        /// @brief Default constructor.
        /// @param c    byte that the Node will contain
        ///
        explicit Node(uint8_t c, CodeType dms):
            first(dms), c(c), left(dms), right(dms)
        {
        }

        CodeType    first;  ///< Code of first child string.
        uint8_t        c;      ///< Byte.
        CodeType    left;   ///< Code of child node with byte < `c`.
        CodeType    right;  ///< Code of child node with byte > `c`.
    };

    CodeType m_dms;
    CodeType m_reserve_dms;
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
    EncoderDictionary(LzMode mode, CodeType dms, CodeType reserve_dms):
        m_dms(dms), m_reserve_dms(reserve_dms), m_lzw_mode(mode == Lzw)
    {
        const long int minc = std::numeric_limits<uint8_t>::min();
        const long int maxc = std::numeric_limits<uint8_t>::max();
        CodeType k {0};

        if (m_lzw_mode) {
            for (long int c = minc; c <= maxc; ++c)
                initials[c] = k++;
        }
        vn.reserve(reserve_dms);
        reset();
    }

    ///
    /// @brief Resets dictionary to its initial contents.
    /// @note Adds dummy nodes to account for the metacodes.
    ///
    void reset()
    {
        vn.clear();

        const long int minc = std::numeric_limits<uint8_t>::min();
        const long int maxc = std::numeric_limits<uint8_t>::max();

        if (m_lzw_mode) {
            for (long int c = minc; c <= maxc; ++c)
                vn.push_back(Node(c, m_dms));
        }

        // add dummy nodes for the metacodes
        vn.push_back(Node('\x00', m_dms)); // MetaCode::Eof
    }

    ///
    /// @brief Searches for a pair (`i`, `c`) and inserts the pair if it wasn't found.
    /// @param i                code to search for
    /// @param c                attached byte to search for
    /// @return The index of the pair, if it was found.
    /// @retval m_dms    if the pair wasn't found
    ///
    CodeType search_and_insert(CodeType i, uint8_t c)
    {
        if (m_lzw_mode) {
            if (i == m_dms)
                return search_initials(c);
        }

        const CodeType vn_size = vn.size();
        CodeType ci {vn[i].first}; // Current Index

        if (ci != m_dms)
        {
            while (true)
                if (c < vn[ci].c)
                {
                    if (vn[ci].left == m_dms)
                    {
                        vn[ci].left = vn_size;
                        break;
                    }
                    else
                        ci = vn[ci].left;
                }
                else
                if (c > vn[ci].c)
                {
                    if (vn[ci].right == m_dms)
                    {
                        vn[ci].right = vn_size;
                        break;
                    }
                    else
                        ci = vn[ci].right;
                }
                else // c == vn[ci].c
                    return ci;
        }
        else
            vn[i].first = vn_size;

        vn.push_back(Node(c, m_dms));
        return m_dms;
    }

    ///
    /// @brief Fakes a search for byte `c` in the one-byte area of the dictionary.
    /// @param c    byte to search for
    /// @return The code associated to the searched byte.
    ///
    CodeType search_initials(uint8_t c) const
    {
        return initials[c];
    }

    ///
    /// @brief Returns the number of dictionary entries.
    ///
    std::vector<Node>::size_type size() const
    {
        return vn.size();
    }

private:

    /// Vector of nodes on top of which the binary search tree is implemented.
    std::vector<Node> vn;

    /// Cheat sheet for mapping one-byte strings to their codes.
    std::array<CodeType, 1u << CHAR_BIT> initials;
};

// TODO: Existing lz78 Coders will not reset dictionary size halfway
// TODO: Existing lz78 DeCoders will not reset dictionary size halfway


}

#endif
