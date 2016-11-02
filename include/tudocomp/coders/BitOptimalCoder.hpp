#ifndef _INCLUDED_BIT_OPTIMAL_CODER_HPP
#define _INCLUDED_BIT_OPTIMAL_CODER_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

class BitOptimalCoder : public Algorithm {

private:
    BitOStream* m_out;

public:
    inline static Meta meta() {
        Meta m("coder", "bitopt", "Bit optimal encoding");
        return m;
    }

    inline BitOptimalCoder() = delete;

    inline BitOptimalCoder(Env&& env, BitOStream& out)
        : Algorithm(std::move(env)), m_out(&out) {
    }

    template<typename range_t>
    inline void encode(uint64_t v, const range_t& r) {
        if(r.min() == 0) {
            m_out->write_int(v, bits_for(r.max()));
        } else {
            m_out->write_int(v - r.min(), bits_for(r.max() - r.min()));
        }
    }

    inline void finalize() {
        m_out->flush();
    }
};

}

#endif
