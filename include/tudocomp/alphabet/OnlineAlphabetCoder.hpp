#ifndef _INCLUDED_ONLINE_ALPHABET_CODER_HPP
#define _INCLUDED_ONLINE_ALPHABET_CODER_HPP

#include <sdsl/int_vector.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/counter.h>

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

    inline void encode_sym(uint8_t sym) {
        DLOG(INFO) << "encode_sym('" << sym << "')";
        m_out->writeBit(0);
        m_out->write(sym);
    }
};

}

#endif
