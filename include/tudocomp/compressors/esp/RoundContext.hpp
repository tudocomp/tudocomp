#pragma once

#include <tudocomp/compressors/esp/TypedBlock.hpp>
#include <tudocomp/compressors/esp/utils.hpp>
#include <tudocomp/compressors/esp/EspContext.hpp>

namespace tdc {namespace esp {
    class BlockGrid {
        std::vector<TypedBlock> m_block_buffer;
        std::array<TypedBlock, 2> m_adjust_buffer;
        size_t m_adjust_buffer_size = 0;
    public:
        BlockGrid() = default;

        inline void push_block(size_t l, size_t type) {
            if (m_adjust_buffer_size == 2) {
                m_block_buffer.push_back(m_adjust_buffer[0]);
                m_adjust_buffer[0] = m_adjust_buffer[1];
                m_adjust_buffer_size = 1;
            }
            m_adjust_buffer[m_adjust_buffer_size++] = TypedBlock { uint8_t(l), uint8_t(type) };

            // Check if the adjust buffer contains a block with invalid size (outside of 2 or 3), and fix it up
            maybe_adjust();
        }

        inline void done() {
            for(size_t i = 0; i < m_adjust_buffer_size; i++) {
                m_block_buffer.push_back(m_adjust_buffer[i]);
            }
            m_adjust_buffer_size = 0;
        }

        // Simplified logic - if there is a block of size 1,
        // merge it with an adjacent one
        inline void maybe_adjust() {
            if (m_adjust_buffer_size == 2) {
                auto& a = m_adjust_buffer[0];
                auto& b = m_adjust_buffer[1];

                if (a.len == 1 || b.len == 1) {
                    size_t combined_len = a.len + b.len;
                    DCHECK((combined_len >= 2) && (combined_len <= 4));
                    if (combined_len == 4) {
                        a.len = 2;
                        b.len = 2;
                    } else {
                        a.len = combined_len;
                        m_adjust_buffer_size--; // remove b
                    }
                }
            }
        }

        inline auto& vec() {
            return m_block_buffer;
        }
    };

    template<typename round_view_t>
    struct RoundContext {
        std::vector<size_t> scratchpad;
        size_t alphabet_size;

        RoundContext(size_t as): alphabet_size(as) {}

        BlockGrid split(round_view_t);
    };
}}
