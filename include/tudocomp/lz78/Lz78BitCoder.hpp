#ifndef _INCLUDED_LZ78_BIT_CODER_HPP_
#define _INCLUDED_LZ78_BIT_CODER_HPP_

#include <tudocomp/Coder.hpp>

#include <tudocomp/lz78/trie.h>
#include <tudocomp/lz78/factor.h>
#include <tudocomp/lz78/factors.h>
#include <tudocomp/lz78/coder.h>
#include <tudocomp/lz78/Lz78DecodeBuffer.hpp>

namespace tudocomp {

namespace lz78 {

/**
 * Encodes factors as simple strings.
 */
class Lz78BitCoder {
private:
    // TODO: Change encode_* methods to not take Output& since that inital setup
    // rather, have a single init location
    tudocomp::io::OutputStreamGuard m_out_guard;
    tudocomp::BitOStream m_out;
    uint64_t m_factor_counter = 0;

public:
    inline Lz78BitCoder(Env& env, Output& out)
        : m_out_guard(out.as_stream()), m_out(*m_out_guard)
    {
    }

    inline Lz78BitCoder(Env& env, Output& out, size_t len)
        : m_out_guard(out.as_stream()), m_out(*m_out_guard)
    {
    }

    inline ~Lz78BitCoder() {
        m_out.flush();
        (*m_out_guard).flush();
    }

    inline void encode_sym(uint8_t sym) {
        throw std::runtime_error("encoder does not support encoding raw symbols");
    }

    inline void encode_fact(const Entry& entry) {
        // output format: variable_number_backref_bits 8bit_char

        // slowly grow the number of bits needed together with the output
        size_t back_ref_idx_bits = bitsFor(m_factor_counter);

        DCHECK(bitsFor(entry.index) <= back_ref_idx_bits);

        m_out.write(entry.index, back_ref_idx_bits);
        m_out.write(entry.chr, 8);

        m_factor_counter++;
    }

    inline static void decode(Input& inp_, Output& out_) {
        Lz78DecodeBuffer buf;

        bool done = false;

        auto i_guard = inp_.as_stream();
        auto o_guard = out_.as_stream();
        auto& out = *o_guard;

        BitIStream inp(*i_guard, done);

        uint64_t factor_counter = 0;

        while (!done) {
            size_t back_ref_idx_bits = bitsFor(factor_counter);

            uint64_t index = inp.readBits<uint64_t>(back_ref_idx_bits);
            uint8_t chr = inp.readBits<uint8_t>(8);

            if (done) {
                break;
            }

            Entry entry { index, chr };

            buf.decode(entry, out);
            factor_counter++;
        }

        out.flush();
    }
};

}

}

#endif
