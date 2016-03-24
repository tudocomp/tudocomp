#ifndef _INCLUDED_LZW_DEBUG_CODER_HPP_
#define _INCLUDED_LZW_DEBUG_CODER_HPP_

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
class LzwDebugCoder {
private:
    // TODO: Change encode_* methods to not take Output& since that inital setup
    // rather, have a single init location
    tudocomp::io::OutputStreamGuard m_out;

public:
    inline LzwDebugCoder(Env& env, Output& out)
        : m_out(out.as_stream())
    {
    }

    inline ~LzwDebugCoder() {
        (*m_out).flush();
    }

    inline void encode_fact(const Factor& entry) {
        if (entry >= 32u && entry <= 127u) {
            (*m_out) << "'" << char(uint8_t(entry)) << "',";
        } else {
            (*m_out) << uint64_t(entry) << ",";
        }
    }

    inline void dictionary_reset() {
        // nothing to be done
    }

    inline static void decode(Input& _inp, Output& _out,
                              CodeType dms,
                              CodeType reserve_dms) {
        auto iguard = _inp.as_stream();
        auto oguard = _out.as_stream();
        auto& inp = *iguard;
        auto& out = *oguard;

        bool more = true;
        char c = '?';
        decode_step([&](CodeType& entry, bool reset, bool &file_corrupted) -> Factor {
            if (!more) {
                return false;
            }

            size_t v;
            more &= parse_number_until_other(inp, c, v);
            DCHECK(v <= std::numeric_limits<CodeType>::max());

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
                return false;
            }
            //std::cout << byte_to_nice_ascii_char(v) << "\n";
            entry = v;
            return true;

        }, out, dms, reserve_dms);
    }
};

}

}

#endif
