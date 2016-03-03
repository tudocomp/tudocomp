#ifndef _INCLUDED_SDSL_ALPHABET_CODER_HPP
#define _INCLUDED_SDSL_ALPHABET_CODER_HPP

#include <sdsl/int_vector.hpp>
#include <sdsl/rrr_vector.hpp>
#include <sdsl/csa_alphabet_strategy.hpp>
#include <sdsl/construct.hpp>

#include <tudocomp/io.h>
#include <tudocomp/util.h>

namespace tudocomp {

class SdslAlphabetCoder {

private:
    using alphabet_type = sdsl::succinct_byte_alphabet<
                            sdsl::rrr_vector<>,
                            sdsl::rrr_vector<>::rank_1_type,
                            sdsl::rrr_vector<>::select_1_type,
                            sdsl::int_vector<>>;

    alphabet_type m_alphabet;
    BitOStream* m_out;

public:
    inline SdslAlphabetCoder(Env& env, BitOStream& out, const boost::string_ref& in) : m_out(&out) {
        //TODO: SOMEHOW get SDSL to swallow the input
        //construct_im(m_alphabet, in.str);
    }
    
    inline void encode_alphabet() {
        //Alphabet size
        m_out->write(m_alphabet.sigma);
        
        //comp2char
        for(size_t x = 0; x < m_alphabet.sigma; x++) {
            m_out->write(m_alphabet.comp2char[x]);
        }
    }
    
    inline void encode_sym(uint8_t c) {
        m_out->write(m_alphabet.char2comp[c], bitsFor(m_alphabet.sigma));
    }
};

}

#endif
