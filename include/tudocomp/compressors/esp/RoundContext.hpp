#pragma once

#include <tudocomp/compressors/esp/utils.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>
#include <tudocomp/compressors/esp/BlockGrid.hpp>

namespace tdc {namespace esp {
    template<typename round_view_t>
    class RoundContext {
        inline size_t search_equal(const round_view_t& src, size_t from) {
            for(size_t j = from; j < src.size() - 1; j++) {
                if (src[j] == src[j + 1]) {
                    return j;
                }
            }
            return src.size();
        }

        inline size_t search_not_equal(const round_view_t& src, size_t from) {
            for(size_t j = from; j < src.size() - 1; j++) {
                if (src[j] != src[j + 1]) {
                    return j + 1;
                }
            }
            return src.size();
        }

        std::vector<size_t> m_scratchpad;
        size_t m_alphabet_size;

    public:
        RoundContext(size_t as): m_alphabet_size(as) {}

        BlockGrid split_into_blocks(round_view_t src)  {
            BlockGrid grid;
            MetablockContext<round_view_t> mbctx {m_scratchpad, grid, m_alphabet_size};

            // Split up the input into metablocks of type 2 or 1/3
            for (size_t i = 0; i < src.size();) {
                size_t j;

                // Scan for non-repeating
                j = search_equal(src, i);
                if(j != i) {
                    mbctx.eager_mb2(src.slice(i, j));
                    i = j;
                }

                // Scan for repeating
                j = search_not_equal(src, i);
                if(j != i) {
                    mbctx.eager_mb13(src.slice(i, j), 1);
                    i = j;
                }
            }

            return grid;
        }
    };
}}
