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
        inline Encoder(Env&& env, Output& out) : tdc::Encoder(std::move(env), out) {}

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            std::cout << "BitOptimalCoder::encode(" << v << ", [" << r.min() << "," << r.max() << "])" << std::endl;
            if(r.min() == 0) {
                m_out->write_int(v, bits_for(r.max()));
            } else {
                m_out->write_int(v - r.min(), bits_for(r.max() - r.min()));
            }
        }
    };

    class Decoder : public tdc::Decoder {
    public:
        inline Decoder(Env&& env, Input& in) : tdc::Decoder(std::move(env), in) {}

        template<typename value_t>
        inline value_t decode(const Range& r) {
            if(r.min() == 0) {
                return m_in->read_int<value_t>(bits_for(r.max()));
            } else {
                return (value_t)r.min() +
                    m_in->read_int<value_t>(bits_for(r.max() - r.min()));
            }
        }
    };
};

}

#endif
