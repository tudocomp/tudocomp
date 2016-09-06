#ifndef _INCLUDED_LZ78_BIT_CODER_HPP_
#define _INCLUDED_LZ78_BIT_CODER_HPP_

#include <tudocomp/lz78/Factor.hpp>
#include <tudocomp/lz78/Lz78DecodeBuffer.hpp>

namespace tudocomp {

namespace lz78 {

/**
 * Encodes factors as simple strings.
 */
class Lz78BitCoder: public Algorithm {
private:
    // TODO: Change encode_* methods to not take Output& since that inital setup
    // rather, have a single init location
    tudocomp::io::OutputStream m_out_guard;
    tudocomp::BitOStream m_out;
    bool empty = false;
    uint64_t m_factor_counter = 0;

    // Stats
    uint32_t m_max_bits_needed_per_factor = 0;

public:
    inline static Meta meta() {
        return Meta("lz78_coder", "bit",
            "Bit coder\n"
            "Basic variable-bit-width encoding of the symbols"
        );
    }

    inline Lz78BitCoder(Env&& env, Output& out):
        Algorithm(std::move(env)),
        m_out_guard(out.as_stream()),
        m_out(m_out_guard)
    {
    }

    inline Lz78BitCoder(Lz78BitCoder&& other):
        Algorithm(std::move(other.env())),
        m_out_guard(std::move(other.m_out_guard)),
        m_out(std::move(other.m_out)),
        m_factor_counter(other.m_factor_counter),
        m_max_bits_needed_per_factor(other.m_max_bits_needed_per_factor) {
            other.empty = true;
        }


    inline ~Lz78BitCoder() {
        if (!empty) {
            m_out.flush();
            m_out_guard.flush();
        }
    }

    inline void encode_fact(const Factor& entry) {
        // output format: variable_number_backref_bits 8bit_char

        // slowly grow the number of bits needed together with the output
        size_t back_ref_idx_bits = bits_for(m_factor_counter);

        DCHECK(bits_for(entry.index) <= back_ref_idx_bits);

        m_out.write(entry.index, back_ref_idx_bits);
        m_out.write(entry.chr, 8);

        m_max_bits_needed_per_factor = std::max<uint32_t>(
                m_max_bits_needed_per_factor, back_ref_idx_bits + 8);

        m_factor_counter++;
    }

    inline void dictionary_reset() {
        m_factor_counter = 0;
    }

    inline static void decode(Input& inp_, Output& out_) {
        Lz78DecodeBuffer buf;

        bool done = false;

        auto i_guard = inp_.as_stream();
        auto out = out_.as_stream();

        BitIStream inp(i_guard, done);

        uint64_t factor_counter = 0;

        while (!done) {
            size_t back_ref_idx_bits = bits_for(factor_counter);

            CodeType index = inp.readBits<CodeType>(back_ref_idx_bits);
            uint8_t chr = inp.readBits<uint8_t>(8);

            if (done) {
                break;
            }

            Factor entry { index, chr };

            buf.decode(entry, out);
            factor_counter++;
        }

        out.flush();
    }
};

}

}

#endif
