#pragma once

#include <tudocomp/compressors/esp/RoundContext.hpp>

namespace tdc {namespace esp {
    template<typename Source, typename F>
    inline size_t split_where(const Source& src, size_t i, bool max, F f) {
        for(size_t j = i; j < src.size() - 1; j++) {
            if (!f(src[j], src[j + 1])) {
                return j + (max ? 1 : 0);
            }
        }
        return src.size();
    }

    template<typename round_view_t>
    void RoundContext<round_view_t>::split(round_view_t src) {
        // Split up the input into metablocks of type 2 or 1/3
        for (size_t i = 0; i < src.size();) {
            size_t j;

            // Scan for non-repeating
            // NB: First to not find a size-1 repeating prefix
            j = split_where(src, i, false,
                            [](size_t a, size_t b){ return a != b; });
            if(j != i) {
                auto s = src.slice(i, j);

                MetablockContext<round_view_t> mbctx {*this};

                mbctx.eager_mb2(s);
                i = j;
            }

            // Scan for repeating
            j = split_where(src, i, true,
                            [](size_t a, size_t b){ return a == b; });
            if(j != i) {
                auto s = src.slice(i, j);

                MetablockContext<round_view_t> mbctx {*this};

                mbctx.eager_mb13(s, 1);
                i = j;
            }
        }
    }
}}
