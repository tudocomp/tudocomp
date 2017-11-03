#pragma once

#include <tudocomp/compressors/esp/TypedBlock.hpp>
#include <tudocomp/compressors/esp/BlockAdjust.hpp>
#include <tudocomp/compressors/esp/utils.hpp>
#include <tudocomp/compressors/esp/EspContext.hpp>

namespace tdc {namespace esp {
    template<typename round_view_t>
    struct RoundContext {
        size_t alphabet_size;
        std::vector<size_t> scratchpad;

        std::vector<TypedBlock> block_buffer;

        RoundContext(size_t as,
                     round_view_t src):
            alphabet_size(as),
            scratchpad()
        {}

        void push_back(size_t l, size_t type) {
            block_buffer.push_back(TypedBlock { uint8_t(l), uint8_t(type) });
        }

        std::vector<TypedBlock>& adjusted_blocks() {
            adjust_blocks(block_buffer);

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
