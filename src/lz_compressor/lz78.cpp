#include <vector>
#include <cmath>
#include <climits>
#include <iostream>
#include <algorithm>

#include "glog/logging.h"
#include "sdsl/int_vector.hpp"

#include "lz78.h"
#include "rule.h"

namespace lz_compressor {

struct Result {
    size_t phrase;
    size_t size;
    Rule rule;
};

struct EdgeNode {
    std::vector<EdgeNode> children;
    size_t phrase;
    size_t index;
    uint8_t chr;

    Result find_or_insert(const boost::string_ref prefix,
                          size_t offset,
                          size_t max_phrase,
                          size_t i)
    {
        if (prefix.size() == 0) {
            size_t ziel = index;
            return { phrase, offset, { i, ziel, offset} };
        } else {
            uint8_t c = prefix[0];

            for (auto& edge : children) {
                if (edge.chr == c) {
                    return edge.find_or_insert(
                        prefix.substr(1), offset + 1, max_phrase, i);
                }
            }

            size_t l = offset;
            size_t ziel = index;

            max_phrase++;
            children.push_back(EdgeNode { {}, max_phrase, i, c });

            return { max_phrase, offset + 1, { i, ziel, l } };
        }
    }
};

/*
 questions:
 - does lz78 allow overlapping? (I think I remember no?)
 */

struct Trie {
    EdgeNode root = EdgeNode { {}, 0, 0, 0 };
    size_t max_phrase = 0;

    Result find_or_insert(const boost::string_ref prefix, size_t i) {
        Result r = root.find_or_insert(prefix, 0, max_phrase, i);
        max_phrase = std::max(max_phrase, r.phrase);
        return r;
    }
};

Rules LZ78Compressor::compress(const Input& input, size_t threshold) {
    boost::string_ref input_ref((const char*)(&input[0]), input.size());
    Trie trie;
    Rules rules;

    for (size_t i = 0; i < input.size(); i++) {
        auto s = input_ref.substr(i);


        Result phrase_and_size = trie.find_or_insert(s, i);

        DLOG(INFO) << "looking at " << input_ref.substr(0, i)
            << "|" << s << " -> " << phrase_and_size.size;

        i += phrase_and_size.size - 1;
        if (phrase_and_size.rule.num >= threshold) {
            rules.push_back(phrase_and_size.rule);
        }
    }

    return rules;
}

}
