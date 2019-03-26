#pragma once

#include <tudocomp/compressors/lz_trie/LZTrie.hpp>
#include <tudocomp/Range.hpp>

namespace tdc {
namespace lz78 {

/// \brief Encodes an LZ78 factor in an online scenario.
/// \tparam coder_t the coder type
/// \param coder the encoder to use
/// \param ref the reference
/// \param cha the following literal
/// \param refnum the current reference number
template<typename coder_t>
inline void encode_factor(
    coder_t& coder,
    size_t ref, uliteral_t cha, size_t refnum) {

    coder.encode(ref, Range(refnum));
    coder.encode(cha, literal_r);
}

class Decompressor {
    std::vector<lz_trie::factorid_t> indices;
    std::vector<uliteral_t> literals;

    public:
    inline void decompress(lz_trie::factorid_t index, uliteral_t literal, std::ostream& out) {
        // enter new factor
        indices.push_back(index);
        literals.push_back(literal);

        // decompress the reference and append new literal
        if(index > 0) {
            decompress_ref(index, out);
        }
        out << literal;
    }

    inline void decompress_ref(lz_trie::factorid_t index, std::ostream& out) {
        // decompress the reference
        std::vector<uliteral_t> buffer;
        uliteral_t literal;

        while(index != 0) {
            literal = literals[index - 1];
            index = indices[index - 1];

            buffer.push_back(literal);
        }

        for(size_t i = buffer.size(); i > 0; i--) {
            out << buffer[i - 1];
        }
    }
};

}}

