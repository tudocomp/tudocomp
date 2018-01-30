#pragma once

#include <tudocomp/Range.hpp>

namespace tdc {
namespace lzss {

/// \brief Encodes an LZSS factor in an online scenario.
/// \param coder the encoder to use
/// \param fpos the current text position
/// \param fsrc the factor's source position
/// \param flen the factor length
/// \param max_len the maximum length of any factor, used for bit compression
template<typename coder_t>
inline void online_encode_factor(
    coder_t& coder,
    size_t fpos, size_t fsrc, size_t flen,
    size_t max_len) {

    coder.encode(true, bit_r);              // 1-bit
    coder.encode(fpos - fsrc, Range(fpos)); // delta
    coder.encode(flen, Range(max_len));     // num
}

/// \brief Encodes a character in an LZSS online scenario.
/// \param coder the encoder to use
/// \param literal the literal to encode
template<typename coder_t>
inline void online_encode_literal(coder_t& coder, uliteral_t literal) {
    coder.encode(false, bit_r);       // 0-bit
    coder.encode(literal, literal_r); // delta
}

}}

