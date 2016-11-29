#ifndef _INCLUDED_ELIAS_GAMMA_CODER_HPP
#define _INCLUDED_ELIAS_GAMMA_CODER_HPP

#include <tudocomp/Coder.hpp>

namespace tdc {

class EliasGammaCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "gamma", "Elias-gamma encoding");
        return m;
    }

    EliasGammaCoder() = delete;

    class Encoder : public tdc::Encoder {
    public:
        ENCODER_CTOR(env, out, literals) {}

        using tdc::Encoder::encode;

        template<typename value_t>
        inline void encode(value_t v, const Range&) {
            encode_elias_gamma(v);
        }
    };

    class Decoder : public tdc::Decoder {
    public:
        DECODER_CTOR(env, in) {}

        using tdc::Decoder::decode;

        template<typename value_t>
        inline value_t decode(const Range&) {
            return decode_elias_gamma<value_t>();
        }
    };
};

}

#endif
