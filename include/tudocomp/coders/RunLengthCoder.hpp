#ifndef _INCLUDED_RUN_LENGTH_CODER_HPP
#define _INCLUDED_RUN_LENGTH_CODER_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

class RunLengthCoder : public Algorithm {

private:
    const ByteRange r_byte;

    uint8_t m_run_byte;
    uint8_t m_run_length;
    BitOStream* m_out;

public:
    inline static Meta meta() {
        Meta m("coder", "rle", "Run-Length encoder");
        return m;
    }

    inline RunLengthCoder() = delete;

    inline RunLengthCoder(Env&& env, BitOStream& out)
        : Algorithm(std::move(env)), m_run_length(0), m_out(&out)  {
    }

    template<typename range_t>
    inline void encode(uint64_t v, const range_t& r) {
        uint64_t max = r.max();
        do {
            encode(v, r_byte);

            v >>= 8U;
            max >>= 8U;
        } while(max);
    }

    inline void emit_run() {
        if(m_run_length > 3) { //TODO option
            m_out->write_int(m_run_byte);
            m_out->write_int(':'); //TODO: needs to be escaped elsewhere
            m_out->write_int(m_run_length); //TODO: width of m_count fixed to bytes
        } else if(m_run_length > 0) {
            while(m_run_length) {
                m_out->write_int(m_run_byte);
                m_run_length--;
            }
        }
    }

    inline void finalize() {
        // final run
        emit_run();

        // flush
        m_out->flush();
    }
};

template<>
inline void RunLengthCoder::encode<ByteRange>(uint64_t v, const ByteRange& r) {
    uint8_t c = uint8_t(v);

    if(m_run_length > 0 && c == m_run_byte) {
        m_run_length++;
        if(m_run_length == 255) emit_run(); //TODO unhardcode?
    } else {
        emit_run();

        // new run
        m_run_byte = c;
        m_run_length = 1;
    }
}

template<>
inline void RunLengthCoder::encode<CharRange>(uint64_t v, const CharRange& r) {
    encode(v, r_byte);
}

}

#endif
