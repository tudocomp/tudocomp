#ifndef _INCLUDED_BYTE_CODER_HPP_
#define _INCLUDED_BYTE_CODER_HPP_

#include <tudocomp/Coder.hpp>

namespace tdc {

class ByteCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "byte", "Simple byte encoding");
        return m;
    }

    ByteCoder() = delete;

    class Encoder : public tdc::Encoder {
    public:
        inline Encoder(Env&& env, Output& out) : tdc::Encoder(std::move(env), out) {}

        template<typename value_t, typename range_t>
        inline void encode(value_t v, const range_t& r) {
            m_out->write_int(v, 8 * bytes_for(bits_for(r.max())));
        }
    };

    class Decoder : public tdc::Decoder {
    public:
        inline Decoder(Env&& env, Input& in) : tdc::Decoder(std::move(env), in) {}

        template<typename value_t, typename range_t>
        inline value_t decode(value_t v, const range_t& r) {
            return m_in->read_int<value_t>(8 * bytes_for(bits_for(r.max())));
        }
    };
};

}

#endif
