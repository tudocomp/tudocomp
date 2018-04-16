#pragma once

#include <tudocomp/Range.hpp>

namespace tdc {
namespace lzss {

/// \brief Encodes an LZSS factor in an online scenario.
/// \tparam coder_t the coder type
/// \param coder the encoder to use
/// \param fpos the current text position
/// \param fsrc the factor's source position
/// \param flen the factor length
/// \param max_len the maximum length of any factor, used for bit compression
template<typename coder_t>
inline void online_encode_factor(
    coder_t& coder,
    size_t fpos, size_t fsrc, size_t flen, Range flen_r) {

    coder.encode(true, bit_r);              // 1-bit
    coder.encode(fpos - fsrc, Range(1, fpos)); // delta
    coder.encode(flen, flen_r);     // num
}

/// \brief Encodes a character in an LZSS online scenario.
/// \tparam coder_t the coder type
/// \param coder the encoder to use
/// \param literal the literal to encode
template<typename coder_t>
inline void online_encode_literal(coder_t& coder, uliteral_t literal) {
    coder.encode(false, bit_r);       // 0-bit
    coder.encode(literal, literal_r); // delta
}

/// \brief Decodes the given input encoded by the online LZSS encoders.
/// \tparam decoder_t the decoder type
/// \param decoder the decoder to use
/// \param max_len the maximum length of any factor
template<typename decoder_t>
inline std::vector<uliteral_t> online_decode(
    decoder_t& decoder, Range flen_r) {

    std::vector<uliteral_t> text;
    while(!decoder.eof()) {
        bool is_factor = decoder.template decode<bool>(bit_r);
        if(is_factor) {
            size_t fsrc = text.size() - decoder.template decode<size_t>(Range(1, text.size()));
            size_t fnum = decoder.template decode<size_t>(flen_r);

            for(size_t i = 0; i < fnum; i++) {
                text.emplace_back(text[fsrc+i]);
            }
        } else {
            auto c = decoder.template decode<uliteral_t>(literal_r);
            text.emplace_back(c);
        }
    }
    return text;
}

}}

