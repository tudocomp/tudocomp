#ifndef _INCLUDED_ONLINE_ALPHABET_CODER_HPP
#define _INCLUDED_ONLINE_ALPHABET_CODER_HPP

#include <sdsl/int_vector.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/DecodeBuffer.hpp>

namespace tudocomp {

//TODO only supports 8-bit characters
class OnlineAlphabetCoder {

private:
    BitOStream* m_out;

public:
    inline OnlineAlphabetCoder(Env& env, Input& input, BitOStream& out) : m_out(&out) {
        //TODO write magic
    }

    inline ~OnlineAlphabetCoder() {
    }
    
    inline void encode_init() {
    }

    inline void encode_sym(uint8_t sym) {
        m_out->writeBit(0);
        m_out->write(sym);
    }
    
    inline void encode_sym_flush() {
    }
    
    template<typename D>
    class Decoder {
    private:
        BitIStream* m_in;
        D* m_buf;
        
    public:
        Decoder(Env& env, BitIStream& in, D& buf) : m_in(&in), m_buf(&buf) {
        }
        
        size_t decode_sym() {
            uint8_t sym = m_in->readBits<uint8_t>();
            m_buf->decode(sym);
            return 1;
        }
    };
};

}

#endif
