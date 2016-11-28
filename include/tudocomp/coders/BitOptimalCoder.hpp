#ifndef _INCLUDED_BIT_OPTIMAL_CODER_HPP
#define _INCLUDED_BIT_OPTIMAL_CODER_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Coder.hpp>

namespace tdc {

class BitOptimalCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "bit", "Bit optimal encoding");
        return m;
    }

    BitOptimalCoder() = delete;

    class Encoder : public tdc::Encoder {
    public:
        ENCODER_CTOR(env, out, literals) {}
    };

    class Decoder : public tdc::Decoder {
    public:
        DECODER_CTOR(env, in) {}
    };
};

}

#endif
