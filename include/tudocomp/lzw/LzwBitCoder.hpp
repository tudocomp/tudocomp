#ifndef _INCLUDED_LZW_BIT_CODER_HPP_
#define _INCLUDED_LZW_BIT_CODER_HPP_

#include <tudocomp/lz78/Lz78DecodeBuffer.hpp>

#include <tudocomp/lzw/Factor.hpp>
#include <tudocomp/lzw/decode.hpp>

namespace tudocomp {

namespace lzw {

using tudocomp::lzw::Factor;
using lz78_dictionary::CodeType;

/**
 * Encodes factors as simple strings.
 */
class LzwBitCoder {
private:
    // TODO: Change encode_* methods to not take Output& since that inital setup
    // rather, have a single init location
    tudocomp::io::OutputStream m_out_guard;
    tudocomp::BitOStream m_out;
    uint64_t m_factor_counter = 0;
    bool empty = false;

public:
    inline static Meta meta() {
        return Meta("lzw_coder", "bit",
            "Bit coder\n"
            "Basic variable-bit-width encoding of the symbols"
        );
    }

    inline LzwBitCoder(LzwBitCoder&& other):
        m_out_guard(std::move(other.m_out_guard)),
        m_out(std::move(other.m_out)),
        m_factor_counter(other.m_factor_counter)
    {
        other.empty = true;
    }

    inline LzwBitCoder(Env&& env, Output& out)
        : m_out_guard(out.as_stream()), m_out(m_out_guard)
    {
    }

    inline ~LzwBitCoder() {
        if (!empty) {
            m_out.flush();
            m_out_guard.flush();
        }
    }

    inline void encode_fact(const Factor& entry) {
        // output format: variable_number_backref_bits 8bit_char

        // slowly grow the number of bits needed together with the output
        size_t back_ref_idx_bits = bits_for(m_factor_counter + 256);

        DCHECK(bits_for(entry) <= back_ref_idx_bits);

        m_out.write(entry, back_ref_idx_bits);

        m_factor_counter++;
    }

    inline void dictionary_reset() {
        m_factor_counter = 0;
    }

    inline static void decode(Input& _inp, Output& _out,
                              CodeType dms,
                              CodeType reserve_dms) {
        auto inp = _inp.as_stream();
        auto out = _out.as_stream();

        bool done = false;
        BitIStream is(inp, done);

        uint64_t counter = 0;
        decode_step([&](CodeType& entry, bool reset, bool &file_corrupted) -> Factor {
            if (reset) {
                counter = 0;
            }

            // Try to read next factor
            Factor factor(is.readBits<uint64_t>(bits_for(counter + 256)));
            if (done) {
                // Could not read all bits -> done
                // (this works because the encoded factors are always > 8 bit)
                return false;
            }
            counter++;
            entry = factor;
            return true;
        }, out, dms, reserve_dms);
    }
};

}

}

#endif
