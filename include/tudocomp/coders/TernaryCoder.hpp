#pragma once

#include <tudocomp/Coder.hpp>

namespace tdc {

class TernaryCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "ternary",
            "Encodes integers using ternary code.");
        return m;
    }

    TernaryCoder() = delete;

    class Encoder : public tdc::Encoder {
    public:
        using tdc::Encoder::Encoder;
        using tdc::Encoder::encode;

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            m_out->write_ternary(v - value_t(r.min()));
        }
    };

    class Decoder : public tdc::Decoder {
    public:
        using tdc::Decoder::Decoder;
        using tdc::Decoder::decode;

        template<typename value_t>
        inline value_t decode(const Range& r) {
            return value_t(r.min()) + m_in->read_ternary<value_t>();
        }
    };
};

}

