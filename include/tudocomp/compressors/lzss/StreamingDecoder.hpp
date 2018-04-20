#pragma once

#include <tudocomp/def.hpp>
#include <tudocomp/Range.hpp>

namespace tdc {
namespace lzss {

template<typename ref_decoder_t, typename len_decoder_t, typename lit_decoder_t>
class StreamingDecoder {
protected:
    std::unique_ptr<ref_decoder_t> m_ref_decoder;
    std::unique_ptr<len_decoder_t> m_len_decoder;
    std::unique_ptr<lit_decoder_t> m_lit_decoder;
    Range m_len_r;

public:
    /// \brief Constructor.
    inline StreamingDecoder(
        std::unique_ptr<ref_decoder_t>&& ref_decoder,
        std::unique_ptr<len_decoder_t>&& len_decoder,
        std::unique_ptr<lit_decoder_t>&& lit_decoder,
        Range len_r = TypeRange<len_compact_t>())
        : m_ref_decoder(std::move(ref_decoder)),
          m_len_decoder(std::move(len_decoder)),
          m_lit_decoder(std::move(lit_decoder)),
          m_len_r(len_r)
    {
    }

    template<typename decomp_t>
    inline void decode(decomp_t& decomp) {
        size_t p = 0;
        while(!m_lit_decoder->eof()) {
            auto is_factor = m_lit_decoder->template decode<bool>(bit_r);
            if(is_factor) {
                size_t fsrc = p - m_ref_decoder->template decode<size_t>(Range(1, p));
                size_t flen = m_len_decoder->template decode<size_t>(m_len_r);

                decomp.decode_factor(fsrc, flen);
                p += flen;
            } else {
                auto c = m_lit_decoder->template decode<uliteral_t>(literal_r);
                decomp.decode_literal(c);
                ++p;
            }
        }
    }
};

}}
