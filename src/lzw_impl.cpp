#include <glog/logging.h>
#include <sdsl/int_vector.hpp>

#include <tudocomp/lzw/lzw_compressor.h>
#include <tudocomp/lzw/lzw_dummy_coder.h>
#include <tudocomp/lzw/lzw_bit_coder.h>

namespace lzw {

using namespace tudocomp;

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

template<class F>
void _decode(F ReadIndex, std::ostream& out) {
    LzwEntries entries;
    std::vector<std::tuple<LzwEntry, uint8_t>> dict;
    for (size_t i = 0; i <= 0xff; i++) {
        dict.push_back(std::tuple<LzwEntry, uint8_t> { 0, i });
    }

    auto AddDict = [&](LzwEntry C, uint8_t x) -> LzwEntry {
        dict.push_back(std::tuple<LzwEntry, uint8_t> { C, x });
        return dict.size() - 1;
    };

    auto GetString = [&](LzwEntry C) -> std::string {
        std::stringstream s;
        while (true) {
            s << std::get<1>(dict[C]);
            C = std::get<0>(dict[C]);
            if (C == 0) {
                break;
            }
        }
        std::string r = s.str();
        return std::string(r.rbegin(), r.rend());
    };

    auto IsIndexInDict = [&](LzwEntry C) -> bool {
        return C < dict.size();
    };

    LzwEntry C = ReadIndex();
    if (C == LzwEntry(-1)) {
        return;
    }
    std::string W = GetString(C);
    out << W;
    //std::cout << W << "\n";
    /*auto f = [&](uint C,
                 uint C_,
                 std::string W,
                 bool indict,
                 uint adddict) {
        std::cout
            << " |\t" << byte_to_nice_ascii_char(C)
            << " |\t" << byte_to_nice_ascii_char(C_)
            << " |\t" << W
            << " |\t" << indict
            << " |\t" << byte_to_nice_ascii_char(adddict)
                << "(" << byte_to_nice_ascii_char(C) << ","
                << W[0] << ")"
            << " |\t" << W << "\n";
    };

    f(C, 0, W, false, 0);*/

    while (C != LzwEntry(-1)) {
        LzwEntry C_ = ReadIndex();

        if (C_ == LzwEntry(-1)) {
            break;
        }

        //LzwEntry new_dict;
        bool isindict;

        if ((isindict = IsIndexInDict(C_))) {
            W = GetString(C_);
            //new_dict = AddDict(C, W[0]);
            AddDict(C, W[0]);
        } else {
            //new_dict = C_ = AddDict(C, W[0]);
            C_ = AddDict(C, W[0]);
            W = GetString(C_);
        }
        out << W;
        //f(C, C_, W, isindict, new_dict);

        C = C_;
    }
}

void LZWDebugCode::decode(Input& _inp, Output& _out) {
    auto iguard = _inp.as_stream();
    auto oguard = _out.as_stream();
    auto& inp = *iguard;
    auto& out = *oguard;

    bool more = true;
    char c = '?';
    _decode([&]() -> LzwEntry {
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

void LZWBitCode::code(LzwEntries&& entries, Output& _out) {
    auto oguard = _out.as_stream();
    auto& out = *oguard;

    // 64 bit = 0-63 => 6 bit
    size_t bit_size = 1;
    for (auto e : entries) {
        bit_size = std::max(bit_size, bitsFor(e));
    }
    DLOG(INFO) << "detected bit_size: " << bit_size;

    BitOstream os(out);

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
    BitIstream is(inp, done);

    uint64_t entries_size = is.readBits<uint64_t>(64);
    uint8_t bit_size = is.readBits<uint64_t>(6) + 1;

    uint64_t counter = 0;
    _decode([&]() -> LzwEntry {
        if (counter == entries_size || done) {
            return LzwEntry(-1);
        }

        counter++;

        return LzwEntry(is.readBits<uint64_t>(bit_size));
    }, out);
}

}
