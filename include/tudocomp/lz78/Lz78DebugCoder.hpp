#ifndef _INCLUDED_LZ78_DEBUG_CODER_HPP_
#define _INCLUDED_LZ78_DEBUG_CODER_HPP_

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
class Lz78DebugCoder {
private:
    // TODO: Change encode_* methods to not take Output& since that inital setup
    // rather, have a single init location
    tudocomp::output::StreamGuard m_out;

public:
    inline Lz78DebugCoder(Env& env, Output& out)
        : m_out(out.as_stream())
    {
    }

    inline Lz78DebugCoder(Env& env, Output& out, size_t len)
        : m_out(out.as_stream())
    {
    }

    inline ~Lz78DebugCoder() {
        (*m_out).flush();
    }

    inline void encode_sym(uint8_t sym) {
        throw std::runtime_error("encoder does not support encoding raw symbols");
    }

    inline void encode_fact(const Entry& fact) {
        *m_out << "(" << fact.index << "," << char(fact.chr) << ")";
    }

    inline static void decode(Input& in, Output& ou) {
        auto i_guard = in.as_stream();
        auto& inp = *i_guard;

        auto o_guard = ou.as_stream();
        auto& out = *o_guard;

        Lz78DecodeBuffer buf;
        char c;

        while (inp.get(c)) {
            DCHECK(c == '(');
            size_t index;
            if (!parse_number_until_other(inp, c, index)) {
                break;
            }
            DCHECK(c == ',');
            inp.get(c);
            char chr = c;
            inp.get(c);
            DCHECK(c == ')');

            buf.decode(Entry { index, uint8_t(chr) }, out);
        }
    }
};

}

}

#endif
