#ifndef LZ78_TRIE_H
#define LZ78_TRIE_H

#include <algorithm>
#include <climits>
#include <cmath>
#include <iostream>
#include <vector>

#include <glog/logging.h>
#include <sdsl/int_vector.hpp>

#include <tudocomp/io.h>
#include <tudocomp/lz78/factor.h>

namespace lz78 {

class PrefixBuffer {
    static const size_t BUFFER_SIZE = 4 * 1024; // 4kb

    std::istream* m_stream;
    //bool m_done = false;
    std::vector<uint8_t> m_buffer;
    size_t m_pos;
    size_t m_end;

    inline std::string slice() {
        std::string s;
        for (size_t i = m_pos; i < m_end; i++) {
            s.push_back(m_buffer[i]);
        }
        return s;
    }

    inline void check_fill_buffer() {
        DLOG(INFO) << "m_pos " << m_pos
                   << " m_end " << m_end
                   << " @ " << slice()
                   << "\n";
        if (m_pos >= m_end) {
            // need to read more
            m_stream->read((char*)&m_buffer[0], BUFFER_SIZE);
            m_end = m_stream->gcount();
            m_pos = 0;
            //m_done |= m_stream->eof();
            DLOG(INFO) << "change to m_pos " << m_pos
                       << " m_end " << m_end
                       << " @ " << slice()
                       << "\n";
        }
    }

public:
    inline PrefixBuffer(std::istream& input)
        : m_stream(&input), m_buffer(BUFFER_SIZE, '0'), m_pos(0), m_end(0) {
        check_fill_buffer();
    }

    inline bool is_empty() {
        check_fill_buffer();
        return (m_pos >= m_end);
    }

    inline uint8_t front_char() {
        check_fill_buffer();
        return m_buffer[m_pos];
    }

    inline void advance_one() {
        m_pos++;
    }
};

using namespace tudocomp;

struct Result {
    size_t size;
    Entry entry;
};

struct EdgeNode {
    std::vector<EdgeNode> children;
    size_t index;
    uint8_t chr;

    inline Entry insert(uint8_t chr, size_t* max_index) {
        size_t new_index = *max_index;
        (*max_index)++;

        Entry new_entry {
            index,
            chr
        };
        children.push_back(EdgeNode { {}, new_index, new_entry.chr });

        return new_entry;
    }

    inline Result find_or_insert(PrefixBuffer& prefix,
                          size_t offset,
                          size_t* max_index,
                          size_t prev_index,
                          bool lzw_mode)
    {
        if (prefix.is_empty() && lzw_mode) {
            return { offset, Entry { index, '$' } };
        } else if (prefix.is_empty()) {
            return { offset, Entry { prev_index, chr } };
        } else {
            uint8_t c = prefix.front_char();

            for (auto& edge : children) {
                if (edge.chr == c) {
                    prefix.advance_one();
                    return edge.find_or_insert(
                        prefix, offset + 1, max_index, index, lzw_mode);
                }
            }

            Entry new_entry = insert(c, max_index);
            return { offset + 1, new_entry };
        }
    }

    inline void print(int ident) {
        DLOG(INFO) << std::setfill(' ') << std::setw(ident)
            << "[" << index << "] " << tudocomp::byte_to_nice_ascii_char(chr) << " [";
        for (auto& e: children) {
            e.print(ident + 4);
        }
        DLOG(INFO) << std::setfill(' ') << std::setw(ident) << "" "]";
    }
};

class Trie {
public:
    enum LzVariant {
        Lz78,
        Lzw,
    };
private:
    EdgeNode root = EdgeNode { {}, 0, 0 };
    size_t max_index = 1;
    LzVariant m_variant;
public:

    Trie(LzVariant v): m_variant(v) {
    }

    inline Result find_or_insert(PrefixBuffer& prefix) {
        bool lzw_mode = m_variant == LzVariant::Lzw;
        Result r = root.find_or_insert(prefix, 0, &max_index, 0, lzw_mode);
        if (!lzw_mode) {
            prefix.advance_one();
        }
        return r;
    }

    inline Entry insert(uint8_t chr) {
        return root.insert(chr, &max_index);
    }

    inline void print(int ident) {
        root.print(0);
    }
};

}

#endif
