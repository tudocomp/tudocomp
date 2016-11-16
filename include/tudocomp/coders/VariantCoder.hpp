#ifndef _INCLUDED_VARIANT_CODER_HPP_
#define _INCLUDED_VARIANT_CODER_HPP_

#include <tudocomp/Coder.hpp>

namespace tdc {

template<
    typename int_coder_t,
    typename lit_coder_t
>
class VariantCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "variant", "Different encoding for integers and literals");
        m.option("int_coder").templated<int_coder_t>();
        m.option("lit_coder").templated<lit_coder_t>();
        return m;
    }

    VariantCoder() = delete;

    class Encoder : public tdc::Encoder {
    private:
        std::unique_ptr<typename int_coder_t::Encoder> m_int_encoder;
        std::unique_ptr<typename lit_coder_t::Encoder> m_lit_encoder;

    public:
        ENCODER_CTOR(env, out, literals),
            m_int_encoder(std::make_unique<typename int_coder_t::Encoder>(
                this->env().env_for_option("int_coder"), out, NoLiterals())),
            m_lit_encoder(std::make_unique<typename lit_coder_t::Encoder>(
                this->env().env_for_option("lit_coder"), out, literals))
        {
        }

        template<typename value_t, typename range_t>
        inline void encode(value_t v, const range_t& r) {
            m_int_encoder->encode(v, r);
        }

        template<typename value_t>
        inline void encode(value_t v, const LiteralRange& r) {
            m_lit_encoder->encode(v, r);
        }
    };

    class Decoder : public tdc::Decoder {
    private:
        std::unique_ptr<typename int_coder_t::Decoder> m_int_decoder;
        std::unique_ptr<typename lit_coder_t::Decoder> m_lit_decoder;

    public:
        DECODER_CTOR(env, in),
            m_int_decoder(std::make_unique<typename int_coder_t::Decoder>(
                this->env().env_for_option("int_coder"), in)),
            m_lit_decoder(std::make_unique<typename lit_coder_t::Decoder>(
                this->env().env_for_option("lit_coder"), in))
        {
        }

        template<typename value_t, typename range_t>
        inline value_t decode(const range_t& r) {
            return m_int_decoder->template decode<value_t>(r);
        }

        template<typename value_t>
        inline value_t decode(const LiteralRange& r) {
            return m_lit_decoder->template decode<value_t>(r);
        }
    };
};

}

#endif
