#pragma once

#include <tudocomp/compressors/esp/RoundContext.hpp>

namespace tdc {namespace esp {
    template<typename round_view_t>
    inline size_t search_equal(const round_view_t& src, size_t from) {
        for(size_t j = from; j < src.size() - 1; j++) {
            if (src[j] == src[j + 1]) {
                return j;
            }
        }
        return src.size();
    }

    template<typename round_view_t>
    inline size_t search_not_equal(const round_view_t& src, size_t from) {
        for(size_t j = from; j < src.size() - 1; j++) {
            if (src[j] != src[j + 1]) {
                return j + 1;
            }
        }
        return src.size();
    }

    template<typename round_view_t>
    BlockGrid RoundContext<round_view_t>::split_into_blocks(round_view_t src) {
        BlockGrid grid;

        // Split up the input into metablocks of type 2 or 1/3
        for (size_t i = 0; i < src.size();) {
            size_t j;

            // Scan for non-repeating
            // NB: First to not find a size-1 repeating prefix
            j = search_equal(src, i);
            if(j != i) {
                auto s = src.slice(i, j);

                MetablockContext<round_view_t> mbctx {*this, grid};

                mbctx.eager_mb2(s);
                i = j;
            }

            // Scan for repeating
            j = search_not_equal(src, i);
            if(j != i) {
                auto s = src.slice(i, j);

                MetablockContext<round_view_t> mbctx {*this, grid};

                mbctx.eager_mb13(s, 1);
                i = j;
            }
        }

        return grid;
    }
}}
