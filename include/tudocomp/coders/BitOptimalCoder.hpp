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

        template<typename range_t>
        inline void encode(uint64_t v, const range_t& r) {
            if(r.min() == 0) {
                m_out->write_int(v, bits_for(r.max()));
            } else {
                m_out->write_int(v - r.min(), bits_for(r.max() - r.min()));
            }
        }
    };
};

}

#endif
