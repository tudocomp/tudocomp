#ifndef _INCLUDED_ELIAS_DELTA_CODER_HPP
#define _INCLUDED_ELIAS_DELTA_CODER_HPP

#include <tudocomp/Coder.hpp>

namespace tdc {

class EliasDeltaCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "delta", "Elias-delta encoding");
        return m;
    }

    EliasDeltaCoder() = delete;

    class Encoder : public tdc::Encoder {
    public:
        ENCODER_CTOR(env, out, literals) {}

        using tdc::Encoder::encode;

        template<typename value_t>
        inline void encode(value_t v, const Range&) {
            m_out->write_elias_delta(v);
        }
    };

    class Decoder : public tdc::Decoder {
    public:
        DECODER_CTOR(env, in) {}

        using tdc::Decoder::decode;

        template<typename value_t>
        inline value_t decode(const Range&) {
            return m_in->read_elias_delta<value_t>();
        }
    };
};

}

#endif
