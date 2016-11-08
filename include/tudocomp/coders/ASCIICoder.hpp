#ifndef _INCLUDED_ASCII_CODER_HPP_
#define _INCLUDED_ASCII_CODER_HPP_

#include <sstream>
#include <tudocomp/Coder.hpp>

namespace tdc {

class ASCIICoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "ascii", "Simple ASCII encoding");
        return m;
    }

    ASCIICoder() = delete;

    class Encoder : public tdc::Encoder {
    public:
        inline Encoder(Env&& env, Output& out) : tdc::Encoder(std::move(env), out) {}

        template<typename range_t>
        inline void encode(uint64_t v, const range_t& r) {
            std::ostringstream s;
            s << v;
            for(uint8_t c : s.str()) m_out->write_int(c);
            m_out->write_int(':');
        }
    };
};

template<>
inline void ASCIICoder::Encoder::encode<CharRange>(uint64_t v, const CharRange& r) {
    m_out->write_int(uint8_t(v));
}

template<>
inline void ASCIICoder::Encoder::encode<BitRange>(uint64_t v, const BitRange& r) {
    m_out->write_int(v ? '1' : '0');
}

}

#endif
