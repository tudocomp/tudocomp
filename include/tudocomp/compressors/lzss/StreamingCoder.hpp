#pragma once

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>
#include <tudocomp/compressors/lzss/LZSSCoder.hpp>

namespace tdc {
namespace lzss {

/// \brief Stream coding strategy.
///
/// This strategy is used for streaming scenarios where the factors are not
/// buffered and nothing is known about the text in advance.
///
/// Naturally, this only allows for references to prior positions in the text.
template<typename ref_coder_t, typename len_coder_t, typename lit_coder_t>
class StreamingCoder : public LZSSCoder<ref_coder_t, len_coder_t, lit_coder_t> {
private:
    using super_t = LZSSCoder<ref_coder_t, len_coder_t, lit_coder_t>;

public:
    inline static Meta meta() {
        return super_t::meta(Meta(
            lzss_coder_type(), "stream", "Streaming / online"));
    }

    using super_t::LZSSCoder;

    template<typename refc_t, typename lenc_t, typename litc_t>
    class Encoder {
    protected:
        std::unique_ptr<refc_t> m_refc;
        std::unique_ptr<lenc_t> m_lenc;
        std::unique_ptr<litc_t> m_litc;
        Range m_flen_r;

    public:
        /// \brief Constructor.
        inline Encoder(
            const Config& cfg,
            std::unique_ptr<refc_t>&& refc,
            std::unique_ptr<lenc_t>&& lenc,
            std::unique_ptr<litc_t>&& litc)
            : m_refc(std::move(refc)),
              m_lenc(std::move(lenc)),
              m_litc(std::move(litc)),
              m_flen_r(LengthRange())
        {
        }

        inline void factor_length_range(Range r) {
            // create a new Range object to avoid type issues when decoding
            m_flen_r = MinDistributedRange(r);
        }

        inline void encode_header() {
            m_lenc->encode(m_flen_r.min(), LengthRange());
            m_lenc->encode(m_flen_r.max(), LengthRange());
            m_lenc->flush();
        }

        inline void encode_factor(Factor f) {
            if(tdc_unlikely(f.pos < f.src)) {
                throw std::runtime_error(
                    "forward references are not allowed");
            }

            m_litc->encode(true, bit_r); // 1-bit to indicate factor
            m_litc->flush(); // context switch
            m_refc->encode(f.pos - f.src, Range(1, f.pos)); // delta
            m_refc->flush(); // context switch
            m_lenc->encode(f.len, m_flen_r);
            m_lenc->flush(); // context switch
        }

        inline void encode_literal(uliteral_t c) {
            m_litc->encode(false, bit_r); // 0-bit to indicate literal
            m_litc->encode(c, literal_r);
            // no context switch here - make good use of potential runs!
        }

        template<typename text_t>
        inline void encode_run(const text_t& text, size_t p, const size_t q) {
            while(p < q) encode_literal(text[p++]);
        }

        template<typename text_t>
        inline void encode_text(
            const text_t& text,
            const FactorBuffer& factors) {

            m_flen_r = factors.factor_length_range();
            factors.encode_text(text, *this);
        }
    };

    template<typename refd_t, typename lend_t, typename litd_t>
    class Decoder {
    protected:
        std::unique_ptr<refd_t> m_refd;
        std::unique_ptr<lend_t> m_lend;
        std::unique_ptr<litd_t> m_litd;

    public:
        /// \brief Constructor.
        inline Decoder(
            const Config& cfg,
            std::unique_ptr<refd_t>&& refd,
            std::unique_ptr<lend_t>&& lend,
            std::unique_ptr<litd_t>&& litd)
            : m_refd(std::move(refd)),
              m_lend(std::move(lend)),
              m_litd(std::move(litd))
        {
        }

        template<typename decomp_t>
        inline void decode(decomp_t& decomp) {
            decomp.initialize(0); // text length not known

            // decode header
            const size_t flen_min = m_lend->template decode<size_t>(LengthRange());
            const size_t flen_max = m_lend->template decode<size_t>(LengthRange());
            MinDistributedRange flen_r(flen_min, flen_max);

            // decode text
            size_t p = 0;
            while(!m_litd->eof()) {
                auto is_factor = m_litd->template decode<bool>(bit_r);
                if(is_factor) {
                    size_t fsrc = p - m_refd->template decode<size_t>(Range(1, p));
                    size_t flen = m_lend->template decode<size_t>(flen_r);

                    decomp.decode_factor(fsrc, flen);
                    p += flen;
                } else {
                    auto c = m_litd->template decode<uliteral_t>(literal_r);
                    decomp.decode_literal(c);
                    ++p;
                }
            }

            decomp.process();
        }
    };

    template<typename literals_t>
    inline auto encoder(Output& output, literals_t&& literals) {
        return super_t::template encoder<Encoder>(output, std::move(literals));
    }

    inline auto decoder(Input& input) {
        return super_t::template decoder<Decoder>(input);
    }
};

}} //ns
