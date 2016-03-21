#include <glog/logging.h>
#include <sdsl/int_vector.hpp>

#include <tudocomp/lzw/lzw_compressor.h>
#include <tudocomp/lzw/dummy_coder.h>
#include <tudocomp/lzw/bit_coder.h>
#include <tudocomp/lzw/decode.hpp>
#include <tudocomp/lz78/dictionary.hpp>

namespace lzw {

using namespace tudocomp;
using ::tudocomp::lzw::decode_step;
using lz78_dictionary::CodeType;
const CodeType dms = lz78_dictionary::DMS_MAX;
const CodeType reserve_dms = 0;

void LZWDebugCode::code(LzwEntries&& entries, Output& _out) {
    auto guard = _out.as_stream();
    auto& out = *guard;

    for (LzwEntry entry : entries) {
        if (entry >= 32u && entry <= 127u) {
            out << "'" << char(uint8_t(entry)) << "',";
        } else {
            out << uint64_t(entry) << ",";
        }
    }
}

void LZWDebugCode::decode(Input& _inp, Output& _out) {
    auto iguard = _inp.as_stream();
    auto oguard = _out.as_stream();
    auto& inp = *iguard;
    auto& out = *oguard;

    bool more = true;
    char c = '?';
    decode_step([&](CodeType& entry, bool reset, bool &file_corrupted) -> LzwEntry {
        if (!more) {
            return false;
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
            return false;
        }
        //std::cout << byte_to_nice_ascii_char(v) << "\n";
        entry = v;
        return true;

    }, out, dms, reserve_dms);
}

void LZWBitCode::code(LzwEntries&& entries, Output& _out) {
    auto oguard = _out.as_stream();
    auto& out = *oguard;

    // 64 bit = 0-63 => 6 bit
    size_t bit_size = 1;
    for (auto e : entries) {
        bit_size = std::max(bit_size, bitsFor(e));
    }
    DLOG(INFO) << "detected bit_size: " << bit_size;

    BitOStream os(out);

    os.write(entries.size(), 64);
    os.write(bit_size - 1, 6);

    for (auto e : entries) {
        os.write(uint64_t(e), bit_size);
    }

    os.flush();

}

void LZWBitCode::decode(Input& _inp, Output& _out) {
    auto iguard = _inp.as_stream();
    auto oguard = _out.as_stream();
    auto& inp = *iguard;
    auto& out = *oguard;

    bool done = false;
    BitIStream is(inp, done);

    uint64_t entries_size = is.readBits<uint64_t>(64);
    uint8_t bit_size = is.readBits<uint64_t>(6) + 1;

    uint64_t counter = 0;
    decode_step([&](CodeType& entry, bool reset, bool &file_corrupted) -> LzwEntry {
        if (counter == entries_size || done) {
            return false;
        }

        counter++;

        entry = LzwEntry(is.readBits<uint64_t>(bit_size));
        return true;
    }, out, dms, reserve_dms);
}

}