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
        ENCODER_CTOR(env, out, literals) {}

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            m_out->write_int(v, 8 * bytes_for(r.max()));
        }
    };

    class Decoder : public tdc::Decoder {
    public:
        DECODER_CTOR(env, in) {}

        template<typename value_t>
        inline value_t decode(const Range& r) {
            return m_in->read_int<value_t>(8 * bytes_for(r.max()));
        }
    };
};

}

#endif
