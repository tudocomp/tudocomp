#ifndef _INCLUDED_BYTE_CODER_HPP_
#define _INCLUDED_BYTE_CODER_HPP_

#include <tudocomp/Coder.hpp>

namespace tdc {

class ByteCoder : public Coder {
public:
    inline static Meta meta() {
        Meta m("coder", "byte", "Simple byte encoding");
        return m;
    }

    inline ByteCoder() = delete;
    inline ByteCoder(Env&& env) : Coder(std::move(env)) {}

    template<typename range_t>
    inline void encode(uint64_t v, const range_t& r) {
        m_out->write_int(v, 8 * bytes_for(bits_for(r.max())));
    }
};

}

#endif
