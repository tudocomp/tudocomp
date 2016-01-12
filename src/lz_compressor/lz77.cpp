#include <deque>
#include <cmath>
#include <climits>
#include <iostream>
#include <iterator>
#include <algorithm>

#include <boost/circular_buffer.hpp>
#include "glog/logging.h"
#include "sdsl/int_vector.hpp"

#include "lz77.h"
#include "lz77rule.h"
#include "rule.h"
#include "rules.h"
#include "bit_iostream.h"
#include "tudocomp_util.h"

namespace lz_compressor {

using namespace tudocomp;
using namespace lz77rule;

Rules LZ77ClassicCompressor::compress(Input& input, size_t threshold) {
    // TODO: Default to realistic values
    size_t dict_cap = env.option_as<size_t>("lz77classic.dict_size", 12);
    size_t preview_cap = env.option_as<size_t>("lz77classic.preview_size", 9) + 1;

    // TODO: Algorithm is naive O(kn) implementation right now.

    //std::cout << dict_cap << "," << preview_cap-1 << std::endl;

    boost::circular_buffer<uint8_t> window(dict_cap + preview_cap);

    auto guard = input.as_stream();
    auto& stream = *guard;
    size_t absolute_pos = 0;
    size_t absolute_size = 0;

    // step 1: fill preview buf
    // step 2: start algo

    size_t dict_size = 0;
    size_t preview_size = 0;

    // Example: [ dict | preview ]
    // [ 12 11 10  9  8  7  6  5  4  3  2  1 | 0  1  2  3  4  5  6  7  8  9 ]
    // index dict by distance from preview part of window
    auto dict = [&](size_t idx) -> uint8_t& {
        return window[dict_size - idx];
    };
    // index preview by distance from current position
    auto preview = [&](size_t idx) -> uint8_t& {
        return window[dict_size + idx];
    };

    auto debug_line = [&]() -> std::string {
        std::stringstream ss;

        for (int i = 0; i < (dict_cap - dict_size); i++) {
            ss << "_";
        }
        for (int i = 0; i < dict_size; i++) {
            ss << window[i]  << "";
        }
        ss << "|";
        for (int i = 0; i < preview_size; i++) {
            ss << window[i + dict_size]  << "";
        }
        for (int i = 0; i < (preview_cap - preview_size); i++) {
            ss << "_";
        }

        return ss.str();
    };

    // try to read n preview bytes, and returns actual count
    auto read_n_preview_bytes = [&](size_t n) -> size_t {
        size_t i = 0;
        char c;
        while (i < n && stream.get(c)) {
            uint8_t byte = c;

            window.push_back(byte);
            if (preview_size < preview_cap) {
                preview_size++;
            }

            ++i;
            ++absolute_size;
        }
        size_t read = i;
        while(dict_size == dict_cap && i < n) {
            window.pop_front();
            i++;
        }
        return read;
    };

    // TODO: optimize to do memcopies where possible, instead of single-byte ones
    read_n_preview_bytes(preview_cap);

    Rules rules;

    while (preview_size > 0) {
        //std::cout << debug_line();

        // remember longest find
        size_t offset = 0;
        size_t length = 0;
        uint8_t next = preview(0);

        for (size_t di = dict_size; di > 0; di--) {
            // compare prefix in dict and preview
            for(size_t i = di, j = 0; ; i--, j++) {
                if (j >= preview_size) {
                    // reached end of input, but are also the largest common prefix
                    if (j > length) {
                        offset = di;
                        length = j;
                        next = '_';
                    }
                    break;
                }
                if (dict(i) != preview(j)) {
                    // remember only largest common prefix
                    if (j > length) {
                        offset = di;
                        length = j;
                        next = preview(j);
                    }
                    break;
                }
            }
        }

        // move window by length + 1 bytes
        size_t actual_read = read_n_preview_bytes(length + 1);
        preview_size -= std::min(length + 1 - actual_read, preview_size);
        dict_size = std::min(dict_cap, dict_size + length + 1);

        //std::cout << " @" << absolute_pos << "/" << absolute_size << " => ";
        //std::cout << "(" << offset << "," << length << "," << char(next) << ")";

        if (length == 0) {
            //std::cout << " => " << char(next) << "\n";
        } else {
            Rule r { absolute_pos, absolute_pos - offset, length };
            rules.push_back(r);
            //std::cout << " => " << r;
            //if (absolute_pos + length != absolute_size) {
                //std::cout << char(next);
            //}
            //std::cout << "\n";
        }

        absolute_pos += length + 1;
    }

    return rules;
}

}
