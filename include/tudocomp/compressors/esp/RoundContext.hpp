#pragma once

#include <tudocomp/compressors/esp/TypedBlock.hpp>
#include <tudocomp/compressors/esp/BlockAdjust.hpp>
#include <tudocomp/compressors/esp/utils.hpp>
#include <tudocomp/compressors/esp/EspContext.hpp>
#include <tudocomp/compressors/esp/DebugContext.hpp>

namespace tdc {namespace esp {
    template<typename round_view_t>
    struct RoundContext {
        DebugRoundContext debug;

        bool behavior_metablocks_maximimze_repeating;
        bool behavior_landmarks_tie_to_right;

        size_t alphabet_size;
        std::vector<size_t> scratchpad;
        round_view_t s;
        size_t i = 0;
        size_t last_i = 0;

        std::vector<TypedBlock> block_buffer;

        RoundContext(size_t as,
                     round_view_t src,
                     bool metablocks_maximimze_repeating,
                     bool landmarks_tie_to_right,
                     DebugRoundContext debug_ctx):
            debug(debug_ctx),
            behavior_metablocks_maximimze_repeating(metablocks_maximimze_repeating),
            behavior_landmarks_tie_to_right(landmarks_tie_to_right),
            alphabet_size(as),
            scratchpad(),
            s(src),
            i(0),
            last_i(0)
        {}

        void debug_check_sizes(string_ref errmsg) {
            IF_DEBUG(
                size_t full_size = 0;
                for (auto& b : block_buffer) {
                    full_size += b.len;
                }
                DCHECK_EQ(full_size, s.size()) << errmsg;
            );
        }

        void push_back(size_t l, size_t type) {
            IF_DEBUG(i += l;)
            block_buffer.push_back(TypedBlock { uint8_t(l), uint8_t(type) });
        }

        void debug_check_advanced(size_t len) {
            IF_DEBUG(
                DCHECK_EQ(i - last_i, len);
                last_i = i;
            )
        }

        std::vector<TypedBlock>& adjusted_blocks() {
            debug_check_sizes("pre adjust");
            adjust_blocks(block_buffer);
            debug_check_sizes("post adjust");
            debug.adjusted_blocks(block_buffer);

            IF_DEBUG(
                for (auto& b: block_buffer) {
                    DCHECK_GE(b.len, 2);
                    DCHECK_LE(b.len, 3);
                }
            );
            return block_buffer;
        }

        void split(round_view_t);
    };
}}
