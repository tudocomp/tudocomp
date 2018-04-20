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

    inline void decode(std::ostream& outs) {
        std::vector<uliteral_t> text;

        while(!m_lit_decoder->eof()) {
            auto is_factor = m_lit_decoder->template decode<bool>(bit_r);
            if(is_factor) {
                size_t fsrc = text.size() - m_ref_decoder
                    ->template decode<size_t>(Range(1, text.size()));

                size_t flen = m_len_decoder->template decode<size_t>(m_len_r);

                for(size_t i = 0; i < flen; i++) {
                    text.emplace_back(text[fsrc+i]);
                }
            } else {
                auto c = m_lit_decoder->template decode<uliteral_t>(literal_r);
                text.emplace_back(c);
            }
        }

        for(auto c : text) outs << c;
    }
};

}}
