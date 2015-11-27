#include <vector>
#include <cmath>
#include <climits>
#include <iostream>
#include <algorithm>

#include "glog/logging.h"
#include "sdsl/int_vector.hpp"

#include "lz78.h"
#include "rule.h"
#include "bit_iostream.h"
#include "tudocomp_util.h"

namespace lz_compressor {

using namespace tudocomp;

struct Result {
    size_t size;
    Entry entry;
};

struct EdgeNode {
    std::vector<EdgeNode> children;
    size_t index;
    uint8_t chr;

    Entry insert(uint8_t chr, size_t* max_index) {
        size_t new_index = *max_index;
        (*max_index)++;

        Entry new_entry {
            index,
            chr
        };
        children.push_back(EdgeNode { {}, new_index, new_entry.chr });

        return new_entry;
    }

    Result find_or_insert(const boost::string_ref prefix,
                          size_t offset,
                          size_t* max_index,
                          size_t prev_index)
    {
        if (prefix.size() == 0) {
            return { offset, Entry { prev_index, chr } };
        } else {
            uint8_t c = prefix[0];

            for (auto& edge : children) {
                if (edge.chr == c) {
                    return edge.find_or_insert(
                        prefix.substr(1), offset + 1, max_index, index);
                }
            }

            Entry new_entry = insert(c, max_index);
            return { offset + 1, new_entry };
        }
    }

    void print(int ident) {
        DLOG(INFO) << std::setfill(' ') << std::setw(ident)
            << "[" << index << "] " << tudocomp::byte_to_nice_ascii_char(chr) << " [";
        for (auto& e: children) {
            e.print(ident + 4);
        }
        DLOG(INFO) << std::setfill(' ') << std::setw(ident) << "" "]";
    }
};

struct Trie {
    EdgeNode root = EdgeNode { {}, 0, 0 };
    size_t max_index = 1;

    Result find_or_insert(const boost::string_ref prefix) {
        Result r = root.find_or_insert(prefix, 0, &max_index, 0);
        return r;
    }

    Entry insert(uint8_t chr) {
        return root.insert(chr, &max_index);
    }
};

Entries compress_impl(const Env& env, const Input& input, bool lzw) {
    boost::string_ref input_ref((const char*)(&input[0]), input.size());
    Trie trie;
    Entries entries;

    if (lzw) {
        for (uint32_t i = 0; i <= 0xff; i++) {
            trie.insert(i);
        }
    }

    for (size_t i = 0; i < input.size(); i++) {
        auto s = input_ref.substr(i);

        Result phrase_and_size = trie.find_or_insert(s);

        DLOG(INFO) << "looking at " << input_ref.substr(0, i)
            << "|" << s << " -> " << phrase_and_size.size;

        i += phrase_and_size.size - 1;

        entries.push_back(phrase_and_size.entry);
    }

    trie.root.print(0);

    return entries;
}

Entries LZ78Compressor::compress(const Input& input) {
    return compress_impl(env, input, false);
}

Entries LZWCompressor::compress(const Input& input) {
    return compress_impl(env, input, true);
}

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

void LZ78DebugCode::code(Entries entries, Input input, std::ostream& out) {
    for (Entry entry : entries) {
        out << "(" << entry.index << "," << char(entry.chr) << ")";
    }
}

void LZ78DebugCode::decode(std::istream& inp, std::ostream& out) {
    Lz78DecodeBuffer buf;
    char c;

    while (inp.get(c)) {
        DCHECK(c == '(');
        size_t index = parse_number_until_other(inp, c);
        DCHECK(c == ',');
        inp.get(c);
        char chr = c;
        inp.get(c);
        DCHECK(c == ')');

        buf.decode(Entry { index, uint8_t(chr) }, out);
    }
}

void LZ78BitCode::code(Entries entries, Input input, std::ostream& out_) {
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

    BitOstream out(out_);

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

void LZ78BitCode::decode(std::istream& inp_, std::ostream& out) {
    Lz78DecodeBuffer buf;

    bool done = false;

    BitIstream inp(inp_, done);

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
