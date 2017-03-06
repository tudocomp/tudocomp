#pragma once

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
        using tdc::Encoder::Encoder;
    };

    class Decoder : public tdc::Decoder {
    public:
        using tdc::Decoder::Decoder;
    };
};

}

