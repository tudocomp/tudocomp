#ifndef _INCLUDED_LZ78_DEBUG_CODER_HPP_
#define _INCLUDED_LZ78_DEBUG_CODER_HPP_

#include <tudocomp/lz78/Factor.hpp>
#include <tudocomp/lz78/Lz78DecodeBuffer.hpp>
#include <glog/logging.h>

namespace tudocomp {

namespace lz78 {

/**
 * Encodes factors as simple strings.
 */
class Lz78DebugCoder: public Algorithm {
private:
    // TODO: Change encode_* methods to not take Output& since that inital setup
    // rather, have a single init location
    tudocomp::io::OutputStream m_out;

    bool empty = false;

public:
    inline static Meta meta() {
        return Meta("lz78_coder", "debug",
            "Debug coder\n"
            "Human readable, comma separated "
            "stream of (integer, char) tuples"
        );
    }

    inline Lz78DebugCoder(Env&& env, Output& out):
        Algorithm(std::move(env)),
        m_out(out.as_stream())
    {
    }

    inline Lz78DebugCoder(Lz78DebugCoder&& other):
        Algorithm(std::move(other.env())),
        m_out(std::move(other.m_out)) {
            other.empty = true;
        }

    inline ~Lz78DebugCoder() {
        if (!empty) {
            (*m_out).flush();
        }
    }

    inline void encode_fact(const Factor& fact) {
        *m_out << "(" << fact.index << "," << char(fact.chr) << ")";
    }

    inline void dictionary_reset() {
        // nothing to be done
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
            DCHECK(index <= std::numeric_limits<CodeType>::max());
            DCHECK(c == ',');
            inp.get(c);
            char chr = c;
            inp.get(c);
            DCHECK(c == ')');

            buf.decode(Factor { CodeType(index), uint8_t(chr) }, out);
        }
    }
};

}

}

#endif
