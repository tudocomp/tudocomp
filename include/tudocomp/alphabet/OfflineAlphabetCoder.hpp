#ifndef _INCLUDED_OFFLINE_ALPHABET_CODER_HPP
#define _INCLUDED_OFFLINE_ALPHABET_CODER_HPP

#include <sdsl/int_vector.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/Counter.hpp>

//TODO only supports 8-bit characters
namespace tudocomp {

class OfflineAlphabetCoder {

private:
    BitOStream* m_out;
    io::InputSliceGuard m_in;
    
    size_t m_sigma;
    size_t m_sigma_bits;
    
    sdsl::int_vector<> m_comp2char;
    sdsl::int_vector<> m_char2comp;

public:
    inline OfflineAlphabetCoder(Env& env, Input& input, BitOStream& out) : m_out(&out), m_in(input.as_view()) {
        Counter<uint8_t> counter;

        for(uint8_t c : *m_in) {
            counter.increase(c);
        }
        
        auto ordered = counter.getSorted();
        
        m_sigma = ordered.size();
        m_sigma_bits = bitsFor(m_sigma - 1);
        
        m_comp2char = sdsl::int_vector<>(m_sigma, 0, 8);
        m_char2comp = sdsl::int_vector<>(255, 0, m_sigma_bits);
        
        for(size_t i = 0; i < m_sigma; i++) {
            uint8_t c = ordered[i].first;
            m_comp2char[i] = c;
            m_char2comp[c] = i;
        }

        //Encode alphabet
        //TODO write magic
        m_out->write_compressed_int(m_sigma);
        for(uint8_t c : m_comp2char) {
            m_out->write(c);
        }
    }

    inline ~OfflineAlphabetCoder() {
    }
    
    inline void encode_sym(uint8_t sym) {
        m_out->writeBit(0);
        m_out->write(uint8_t(m_char2comp[sym]), m_sigma_bits);
    }
    
    inline void encode_sym_flush() {
    }
};

}

#endif
