#ifndef _INCLUDED_BYTE_CODER_HPP
#define _INCLUDED_BYTE_CODER_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

class ByteCoder : public Algorithm {

private:
    BitOStream* m_out;

public:
    inline static Meta meta() {
        Meta m("coder", "byte", "Simple byte encoding");
        return m;
    }

    inline ByteCoder() = delete;

    inline ByteCoder(Env&& env, BitOStream& out)
        : Algorithm(std::move(env)), m_out(&out) {
    }

    template<typename range_t>
    inline void encode(uint64_t v, const range_t& r) {
        m_out->write_int(v, 8 * bytes_for(bits_for(r.max())));
    }

    inline void finalize() {
        m_out->flush();
    }
};

}

#endif
