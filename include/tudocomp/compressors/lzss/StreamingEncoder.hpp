#pragma once

#include <tudocomp/def.hpp>
#include <tudocomp/Range.hpp>

#include <tudocomp/compressors/lzss/Factor.hpp>

namespace tdc {
namespace lzss {

template<typename ref_coder_t, typename len_coder_t, typename lit_coder_t>
class StreamingEncoder {
protected:
    std::unique_ptr<ref_coder_t> m_ref_coder;
    std::unique_ptr<len_coder_t> m_len_coder;
    std::unique_ptr<lit_coder_t> m_lit_coder;
    Range m_len_r;

public:
    /// \brief Constructor.
    inline StreamingEncoder(
        std::unique_ptr<ref_coder_t>&& ref_coder,
        std::unique_ptr<len_coder_t>&& len_coder,
        std::unique_ptr<lit_coder_t>&& lit_coder,
        Range len_r = TypeRange<len_compact_t>())
        : m_ref_coder(std::move(ref_coder)),
          m_len_coder(std::move(len_coder)),
          m_lit_coder(std::move(lit_coder)),
          m_len_r(len_r)
    {
    }

    inline void encode_factor(Factor f) {
        if(tdc_unlikely(f.pos < f.src)) {
            throw std::runtime_error(
                "forward references are not allowed");
        }

        m_lit_coder->encode(true, bit_r); // 1-bit to indicate factor
        m_lit_coder->flush(); // context switch
        m_ref_coder->encode(f.pos - f.src, Range(1, f.pos)); // delta
        m_ref_coder->flush(); // context switch
        m_len_coder->encode(f.len, m_len_r);
        m_len_coder->flush(); // context switch
    }

    inline void encode_literal(uliteral_t c) {
        m_lit_coder->encode(false, bit_r); // 0-bit to indicate literal
        m_lit_coder->encode(c, literal_r);
        // no context switch here - make good use of potential runs!
    }
};

}}
