#ifndef _INCLUDED_LZW_DEBUG_CODER_HPP_
#define _INCLUDED_LZW_DEBUG_CODER_HPP_

#include <tudocomp/Coder.hpp>

#include <tudocomp/lz78/trie.h>
#include <tudocomp/lz78/Lz78DecodeBuffer.hpp>

#include <tudocomp/lzw/factor.h>

namespace tudocomp {

namespace lzw {

using ::lzw::LzwEntry;

/**
 * Encodes factors as simple strings.
 */
class LzwDebugCoder {
private:
    // TODO: Change encode_* methods to not take Output& since that inital setup
    // rather, have a single init location
    tudocomp::output::StreamGuard m_out;

public:
    inline LzwDebugCoder(Env& env, Output& out)
        : m_out(out.as_stream())
    {
    }

    inline LzwDebugCoder(Env& env, Output& out, size_t len)
        : m_out(out.as_stream())
    {
    }

    inline ~LzwDebugCoder() {
        (*m_out).flush();
    }

    inline void encode_sym(uint8_t sym) {
        throw std::runtime_error("encoder does not support encoding raw symbols");
    }

    inline void encode_fact(const LzwEntry& entry) {
        if (entry >= 32u && entry <= 127u) {
            (*m_out) << "'" << char(uint8_t(entry)) << "',";
        } else {
            (*m_out) << uint64_t(entry) << ",";
        }
    }

    inline static void decode(Input& _inp, Output& _out) {
        auto iguard = _inp.as_stream();
        auto oguard = _out.as_stream();
        auto& inp = *iguard;
        auto& out = *oguard;

        bool more = true;
        char c = '?';
        decode_step([&]() -> LzwEntry {
            if (!more) {
                return LzwEntry(-1);
            }

            LzwEntry v;
            more &= parse_number_until_other(inp, c, v);

            if (more && c == '\'') {
                more &= bool(inp.get(c));
                v = uint8_t(c);
                more &= bool(inp.get(c));
                DCHECK(c == '\'');
                more &= bool(inp.get(c));
            }

            if (more) {
                DCHECK(c == ',');
            }

            if (!more) {
                return LzwEntry(-1);
            }
            //std::cout << byte_to_nice_ascii_char(v) << "\n";
            return v;

        }, out);
    }
};

}

}

#endif
