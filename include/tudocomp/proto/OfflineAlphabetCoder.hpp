#ifndef _INCLUDED_OFFLINE_ALPHABET_CODER_HPP
#define _INCLUDED_OFFLINE_ALPHABET_CODER_HPP

#include <sdsl/int_vector.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/counter.h>

namespace tudocomp {

class OfflineAlphabetCoder {

private:
    BitOStream* m_out;
    
    size_t m_sigma;
    sdsl::int_vector<> m_comp2char;
    sdsl::int_vector<> m_char2comp;

public:
    inline OfflineAlphabetCoder(Env& env, BitOStream& out, const boost::string_ref& in) : m_out(&out) {
        Counter<uint8_t> counter;
        
        for(uint8_t c : in) {
            counter.increase(c);
        }
        
        auto ordered = counter.getSorted();
        
        m_sigma = ordered.size();
        m_comp2char = sdsl::int_vector<>(m_sigma, 0, 8);
        m_char2comp = sdsl::int_vector<>(255, 0, bitsFor(m_sigma));
        
        for(size_t i = 0; i < m_sigma; i++) {
            uint8_t c = ordered[i].first;
            m_comp2char[i] = c;
            m_char2comp[c] = i;
        }
    }
    
    inline void encode_alphabet() {
        m_out->write(m_sigma, sizeof(m_sigma));
        for(uint8_t c : m_comp2char) {
            m_out->write(c, 8);
        }
    }
    
    inline void encode_syms(const boost::string_ref& in, size_t start, size_t num) {
        for(size_t p = start; p < start + num; p++) {
            uint8_t c = in[p];
            m_out->write((uint8_t)m_char2comp[c], bitsFor(m_sigma));
        }
    }
};

}

#endif
