#ifndef _INCLUDED_BIT_CODER_HPP
#define _INCLUDED_BIT_CODER_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Coder.hpp>

namespace tdc {

class BitCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "bit", "Encoding using range-optimal bit widths");
        return m;
    }

    BitCoder() = delete;

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
