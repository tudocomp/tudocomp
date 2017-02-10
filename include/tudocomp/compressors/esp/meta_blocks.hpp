#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/pre_header.hpp>

namespace tdc {
namespace esp {

template<typename Source, typename Target>
inline void split_repeating(const Source& src, Target& target, Context& ctx) {
    size_t repeating_length = 0;
    size_t max_lookahead = std::min(src.size(), size_t(5));
    for (; repeating_length < max_lookahead; repeating_length++) {
        if (src[0] != src[repeating_length]) {
            break;
        }
    }

    // We want minimum-length repeats
    repeating_length--;

    switch (repeating_length) {
        case 5:
            target.push_back(3);
            break;
        case 4:
            target.push_back(2);
            break;
        case 3:
            target.push_back(3);
            break;
        case 2:
            target.push_back(2);
            break;
        default:
            DCHECK_GT(repeating_length, 1);
    }
}

template<typename Source, typename Target>
inline void eager_mb_2(const Source& src, Target& target, Context& ctx) {

    target.push_back(src.size());
}

template<typename Source, typename Target>
inline void split_non_repeating(const Source& src, Target& target, Context& ctx) {
    size_t non_repeating_length = 0;
    for (; non_repeating_length < src.size() - 1; non_repeating_length++) {
        if (src[non_repeating_length] == src[non_repeating_length + 1]) {
            break;
        }
    }

    // we want maximum-length non-repeats
    non_repeating_length++;

    eager_mb_2(src.substr(0, non_repeating_length), target, ctx);
}

template<typename Source, typename Target>
inline void split(const Source& src, Target& target, Context& ctx) {
    DCHECK_GT(src.size(), 1);

    if (src[0] == src[1]) {
        split_repeating(src, target, ctx);
    } else {
        split_non_repeating(src, target, ctx);
    }
}

}
}
