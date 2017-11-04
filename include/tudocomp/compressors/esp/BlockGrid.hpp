#pragma once

namespace tdc {namespace esp {
    struct TypedBlock {
        uint8_t len;
        uint8_t type;
    };

    inline bool operator==(const TypedBlock& a, const TypedBlock& b) {
        return a.len == b.len && a.type == b.type;
    }

    inline std::ostream& operator<<(std::ostream& o, const TypedBlock& b) {
        return o << "{ len: " << int(b.len) << ", type: " << int(b.type) << " }";
    }

    class BlockGrid {
        // Store the bulk of blocks as one bit each:
        // bit == 0: block len == 2
        // bit == 1: block len == 3
        BitVector m_block_buffer;
        std::array<TypedBlock, 2> m_adjust_buffer;
        size_t m_adjust_buffer_size = 0;
    public:
        BlockGrid() = default; // TODO: add ability to preallocate?

        inline void push_block(size_t l, size_t type) {
            if (m_adjust_buffer_size == 2) {
                DCHECK(m_adjust_buffer[0].len >= 2 && m_adjust_buffer[0].len <= 3);
                m_block_buffer.push_back(m_adjust_buffer[0].len - 2);
                m_adjust_buffer[0] = m_adjust_buffer[1];
                m_adjust_buffer_size = 1;
            }
            m_adjust_buffer[m_adjust_buffer_size++] = TypedBlock { uint8_t(l), uint8_t(type) };

            // Check if the adjust buffer contains a block with invalid size (outside of 2 or 3), and fix it up
            maybe_adjust();
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

        template<typename F>
        inline void for_each_block_len(F f) {
            for (auto e : m_block_buffer) {
                f(e + 2); // convert bit to real block width
            }
            for(size_t i = 0; i < m_adjust_buffer_size; i++) {
                f(m_adjust_buffer[i].len);
            }
        }
    };
}}
