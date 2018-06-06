#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Coder.hpp>

namespace tdc {

class SLEIntCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "sle",
           "Static low entropy encoding, conforming [Dinklage, 2015]."
        );
        return m;
    }

    SLEIntCoder() = delete;

    class Encoder : public tdc::Encoder {
    public:
        using tdc::Encoder::Encoder;
        using tdc::Encoder::encode;

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            // see [Dinklage, 2015]
            v -= r.min();
            auto bits = bits_for(r.delta());

            if(bits <= 5) {
                m_out->write_int(v, bits);
            } else {
                if(v < 8u) {
                    m_out->write_int(0, 2);
                    m_out->write_int(v, 3);
                } else if(v < 16u) {
                    m_out->write_int(1, 2);
                    m_out->write_int(v - value_t(8), 3);
                } else if(v < 32u) {
                    m_out->write_int(2, 2);
                    m_out->write_int(v - value_t(16), 4);
                } else {
                    m_out->write_int(3, 2);
                    m_out->write_int(v, bits);
                }
            }
        }
    };

    class Decoder : public tdc::Decoder {
    public:
    public:
        using tdc::Decoder::Decoder;
        using tdc::Decoder::decode;

		template<typename value_t>
		inline value_t decode(const Range& r) {
            auto bits = bits_for(r.delta());
            value_t v = 0;

            if(bits <= 5) {
                v = m_in->read_int<value_t>(bits);
            } else {
                auto x = m_in->read_int<uint8_t>(2);
                switch(x) {
                    case 0: v = m_in->read_int<value_t>(3); break;
                    case 1: v = value_t(8) + m_in->read_int<value_t>(3); break;
                    case 2: v = value_t(16) + m_in->read_int<value_t>(4); break;
                    case 3: v = m_in->read_int<value_t>(bits); break;
                }
            }

            return v + value_t(r.min());
		}
    };
};

}

