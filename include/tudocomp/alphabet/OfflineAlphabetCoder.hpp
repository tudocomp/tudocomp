#ifndef _INCLUDED_OFFLINE_ALPHABET_CODER_HPP
#define _INCLUDED_OFFLINE_ALPHABET_CODER_HPP

#include <sdsl/int_vector.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/Counter.hpp>
#include <tudocomp/Algorithm.hpp>

//TODO only supports 8-bit characters
namespace tudocomp {

class OfflineAlphabetCoder: Algorithm {

private:
    Env* m_env;

    BitOStream* m_out;
    io::InputView m_in;

    size_t m_sigma;
    size_t m_sigma_bits;

    sdsl::int_vector<> m_comp2char;
    sdsl::int_vector<> m_char2comp;

public:
    inline static Meta meta() {
        Meta m("alpha_coder", "offline",
            "Offline alphabet coder\n"
            "Optimized symbol encoding using alphabet statistics"
        );
        return m;
    }

    inline OfflineAlphabetCoder(Env&& env, Input& input, BitOStream& out):
        Algorithm(std::move(env)), m_out(&out), m_in(input.as_view()) {
        this->env().begin_stat_phase("Analyze alphabet");

        Counter<uint8_t> counter;

        for(uint8_t c : m_in) {
            counter.increase(c);
        }

        auto ordered = counter.getSorted();

        m_sigma = ordered.size();
        m_sigma_bits = bits_for(m_sigma - 1);

        m_comp2char = sdsl::int_vector<>(m_sigma, 0, 8);
        m_char2comp = sdsl::int_vector<>(255, 0, m_sigma_bits);

        for(size_t i = 0; i < m_sigma; i++) {
            uint8_t c = ordered[i].first;
            m_comp2char[i] = c;
            m_char2comp[c] = i;
        }

        this->env().log_stat("alphabetSize", m_sigma);
        this->env().end_stat_phase();
    }

    inline ~OfflineAlphabetCoder() {
    }

    inline void encode_init() {
        //Encode alphabet
        m_out->write_compressed_int(m_sigma);
        for(uint8_t c : m_comp2char) {
            m_out->write_int(c);
        }
    }

    inline void encode_sym(uint8_t sym) {
        m_out->write_bit(0);
        m_out->write_int(uint8_t(m_char2comp[sym]), m_sigma_bits);
    }

    inline void encode_sym_flush() {
    }

    template<typename D>
    class Decoder {
    private:
        BitIStream* m_in;
        D* m_buf;

        size_t m_sigma_bits;
        sdsl::int_vector<> m_comp2char;

    public:
        Decoder(Env& env, BitIStream& in, D& buf) : m_in(&in), m_buf(&buf) {

            size_t sigma = in.read_compressed_int();

            m_sigma_bits = bits_for(sigma - 1);
            m_comp2char = sdsl::int_vector<>(sigma, 0, 8);

            //Decode alphabet
            for(size_t i = 0; i < sigma; i++) {
                uint8_t c = in.read_int<uint8_t>();
                m_comp2char[i] = c;
            }
        }

        size_t decode_sym() {
            uint8_t sym = m_comp2char[m_in->read_int<uint8_t>(m_sigma_bits)];
            m_buf->decode(sym);
            return 1;
        }
    };
};

}

#endif
