#pragma once

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>
#include <tudocomp/compressors/lzss/LZSSCoder.hpp>

namespace tdc {
namespace lzss {

/// \brief Buffered bidirectional coding strategy.
///
/// This strategy encodes the factorization in a succinct manner after
/// an analyzation step.
///
/// It allows both left and right references. Each run of literals is preceded
/// by the run's length.
template<typename ref_coder_t, typename len_coder_t, typename lit_coder_t>
class BufferedBidirectionalCoder : public LZSSCoder<ref_coder_t, len_coder_t, lit_coder_t> {
private:
    using super_t = LZSSCoder<ref_coder_t, len_coder_t, lit_coder_t>;

public:
    inline static Meta meta() {
        return super_t::meta(Meta("lzss_coder", "bi", "Buffered bidirectional coding"));
    }

    using super_t::LZSSCoder;

    template<typename refc_t, typename lenc_t, typename litc_t>
    class Encoder {
    protected:
        std::unique_ptr<refc_t> m_refc;
        std::unique_ptr<lenc_t> m_lenc;
        std::unique_ptr<litc_t> m_litc;
        Range m_flen_r, m_run_r, m_ref_r;

    public:
        /// \brief Constructor.
        inline Encoder(
            const Env& env,
            std::unique_ptr<refc_t>&& refc,
            std::unique_ptr<lenc_t>&& lenc,
            std::unique_ptr<litc_t>&& litc)
            : m_refc(std::move(refc)),
              m_lenc(std::move(lenc)),
              m_litc(std::move(litc)),
              m_flen_r(LengthRange()),
              m_run_r(LengthRange()),
              m_ref_r(LengthRange())
        {
        }

        inline void encode_header() {
            m_lenc->encode(m_ref_r.max(), LengthRange());
            m_lenc->encode(m_flen_r.min(), m_ref_r);
            m_lenc->encode(m_flen_r.max(), m_ref_r);
            m_lenc->encode(m_run_r.max(), m_ref_r);
            m_lenc->flush();
        }

        inline void encode_factor(Factor f) {
            m_litc->encode(true, bit_r);
            m_litc->flush();
            m_refc->encode(f.src, m_ref_r);
            m_refc->flush();
            m_lenc->encode(f.len, m_flen_r);
            m_lenc->flush();
        }

        template<typename text_t>
        inline void encode_run(const text_t& text, size_t p, const size_t q) {
            if(p < q) {
                m_litc->encode(false, bit_r);
                m_litc->flush();
                m_lenc->encode(q-p, m_run_r);
                m_lenc->flush();

                while(p < q) {
                    m_litc->encode(text[p++], uliteral_r);
                }

                m_litc->flush();
            }
        }

        template<typename text_t, typename factorbuffer_t>
        inline void encode_text(
            const text_t& text,
            const factorbuffer_t& factors) {

            m_ref_r = Range(text.size());

            // create a new Range object to avoid type issues when decoding
            m_flen_r = MinDistributedRange(factors.factor_length_range());

            // analyze factorization
            size_t longest_run = 0;
            size_t p = 0;
            for(auto& f : factors) {
                longest_run = std::max(longest_run, f.pos - p);
                p = f.pos + f.len;
            }
            longest_run = std::max(longest_run, text.size() - p);
            m_run_r = MinDistributedRange(1, longest_run);

            // encode
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
            const Env& env,
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
            // decode header
            const size_t n = m_lend->template decode<size_t>(LengthRange());
            Range ref_r(n);

            decomp.initialize(n);

            const size_t flen_min = m_lend->template decode<size_t>(ref_r);
            const size_t flen_max = m_lend->template decode<size_t>(ref_r);
            MinDistributedRange flen_r(flen_min, flen_max);

            const size_t longest_run = m_lend->template decode<size_t>(ref_r);
            MinDistributedRange run_r(1, longest_run);

            // decode text
            while(!m_litd->eof()) {
                auto is_factor = m_litd->template decode<bool>(bit_r);
                if(is_factor) {
                    size_t fsrc = m_refd->template decode<size_t>(ref_r);
                    size_t flen = m_lend->template decode<size_t>(flen_r);

                    decomp.decode_factor(fsrc, flen);
                } else {
                    size_t run = m_lend->template decode<size_t>(run_r);
                    while(run--) {
                        auto c = m_litd->template decode<uliteral_t>(literal_r);
                        decomp.decode_literal(c);
                    }
                }
            }

            decomp.process();
        }
    };

    inline void factor_length_range(Range) {
        // ignore (use range from factor buffer)
    }

    template<typename literals_t>
    inline auto encoder(Output& output, literals_t&& literals) {
        return super_t::template encoder<Encoder>(output, std::move(literals));
    }

    inline auto decoder(Input& input) {
        return super_t::template decoder<Decoder>(input);
    }
};

}}
