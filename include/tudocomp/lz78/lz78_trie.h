#ifndef LZ78_TRIE_H
#define LZ78_TRIE_H

#include <vector>
#include <cmath>
#include <climits>
#include <iostream>
#include <algorithm>

#include <glog/logging.h>
#include <sdsl/int_vector.hpp>

#include <tudocomp/io.h>
#include <tudocomp/lz78/lz78_factor.h>

namespace lz78 {

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

    inline Result find_or_insert(const boost::string_ref prefix,
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

    inline void print(int ident) {
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

    inline Result find_or_insert(const boost::string_ref prefix) {
        Result r = root.find_or_insert(prefix, 0, &max_index, 0);
        return r;
    }

    inline Entry insert(uint8_t chr) {
        return root.insert(chr, &max_index);
    }
};

}

#endif
