#include <glog/logging.h>
#include <sdsl/int_vector.hpp>

#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lz78/lz78_compressor.h>
#include <tudocomp/lz78/dummy_coder.h>
#include <tudocomp/lz78/bit_coder.h>

namespace lz78 {

using namespace tudocomp;

struct Lz78DecodeBuffer {
    Entries dict;
    std::vector<uint8_t> dict_walk_buf;

    void decode(Entry entry, std::ostream& out) {
        dict.push_back(entry);

        size_t index = entry.index;
        uint8_t chr = entry.chr;

        dict_walk_buf.clear();

        while (index != 0) {
            dict_walk_buf.push_back(chr);
            chr = Entry(dict[index - 1]).chr;
            index = Entry(dict[index - 1]).index;
        }

        out << chr;
        for (size_t i = dict_walk_buf.size(); i > 0; i--) {
            out << dict_walk_buf[i - 1];
        }
    }
};

void LZ78DebugCode::code(Entries&& entries, Output& out) {
    auto guard = out.as_stream();

    for (Entry entry : entries) {
        *guard << "(" << entry.index << "," << char(entry.chr) << ")";
    }
}

void LZ78DebugCode::decode(Input& in, Output& ou) {
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

void LZ78BitCode::code(Entries&& entries, Output& out_) {
    // only store as many bits for a index and a chr as necessary.
    // - indices will always start around 0, so just limit the size for them
    // - chars might start and end in a narrow range, so use only that.

    size_t max_index = 0;
    size_t max_chr = 0;
    size_t min_chr = 0xff;

    for (Entry entry : entries) {
        max_index = std::max(entry.index, max_index);
        max_chr = std::max(size_t(uint8_t(entry.chr)), max_chr);
        min_chr = std::min(size_t(uint8_t(entry.chr)), min_chr);
    }

    // In case we have no rules...
    if (min_chr > max_chr) { min_chr = max_chr; }

    size_t chr_offset = min_chr;
    size_t chr_range = max_chr - min_chr;

    size_t index_bits = bitsFor(max_index);
    size_t chr_bits = bitsFor(chr_range);

    DLOG(INFO) << "chr_offset " << chr_offset;
    DLOG(INFO) << "chr_range " << chr_range;
    DLOG(INFO) << "index_bits " << index_bits;
    DLOG(INFO) << "chr_bits " << chr_bits;

    auto guard = out_.as_stream();

    BitOStream out(*guard);

    out.write<uint64_t>(entries.size());
    out.write<uint8_t>(index_bits);
    out.write<uint8_t>(chr_bits);
    out.write<uint8_t>(chr_offset);

    for (Entry entry : entries) {
        out.write(entry.index, index_bits);
        out.write(entry.chr - chr_offset, chr_bits);
    }

    out.flush();
}

void LZ78BitCode::decode(Input& inp_, Output& out_) {
    Lz78DecodeBuffer buf;

    bool done = false;

    auto i_guard = inp_.as_stream();
    auto o_guard = out_.as_stream();
    auto& out = *o_guard;

    BitIStream inp(*i_guard, done);

    uint64_t size = inp.readBits<uint64_t>();
    uint8_t index_bits = inp.readBits<uint8_t>();
    uint8_t chr_bits = inp.readBits<uint8_t>();
    uint8_t chr_offset = inp.readBits<uint8_t>();

    DLOG(INFO) << "size " << size;
    DLOG(INFO) << "chr_offset " << uint(chr_offset);
    DLOG(INFO) << "index_bits " << uint(index_bits);
    DLOG(INFO) << "chr_bits " << uint(chr_bits);

    uint64_t i = 0;
    while (i < size && !done) {
        size_t index = inp.readBits<size_t>(index_bits);
        uint8_t chr = inp.readBits<uint8_t>(chr_bits) + chr_offset;

        Entry entry { index, chr };

        buf.decode(entry, out);
        i++;
    }

    out.flush();
}

}
