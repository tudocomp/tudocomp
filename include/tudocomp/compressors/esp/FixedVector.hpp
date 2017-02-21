#pragma once

#include <array>

namespace tdc {namespace esp {
    template<typename T, size_t N>
    class FixedVector {
        std::array<T, N> m_array;
        size_t m_size = 0;
    public:
        inline FixedVector() {}
        inline void push_back(T&& t) {
            DCHECK_LT(m_size, N);
            m_array[m_size] = std::move(t);
            m_size++;
        }
        inline T pop_back() {
            DCHECK_GT(m_size, 0);
            m_size--;
            return std::move(m_array[m_size]);
        }
        inline T pop_front() {
            DCHECK_GT(m_size, 0);
            auto e = std::move(m_array[0]);

            for(size_t i = 0; i < m_size - 1; i++) {
                m_array[i] = m_array[i + 1];
            }
            m_size--;

            return std::move(e);
        }
        inline bool full() {
            return m_size == N;
        }
        inline GenericView<T> view() {
            return GenericView<T> { m_array.data(), m_size };
        }
    };
}}
