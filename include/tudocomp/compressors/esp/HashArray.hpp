#pragma once

#include<array>

#include<tudocomp/util/View.hpp>

namespace tdc {namespace esp {
    using in_t = ConstGenericView<size_t>;

    template<size_t N, typename T = size_t>
    struct Array {
        using in_t = ConstGenericView<T>;

        std::array<T, N> m_data;
        Array() {
            for(size_t i = 0; i < N; i++) {
                m_data[i] = 0;
            }
        }
        Array(in_t v) {
            DCHECK_EQ(v.size(), N);
            for(size_t i = 0; i < N; i++) {
                m_data[i] = v[i];
            }
        }
        Array(const std::array<T, N>& v): Array(in_t(v)) {}

        inline in_t as_view() const {
            return in_t(m_data);
        }
    };

    template<size_t N, typename T>
    bool operator==(const Array<N, T>& lhs, const Array<N, T>& rhs) {
        for(size_t i = 0; i < N; i++) {
            if (lhs.m_data[i] != rhs.m_data[i]) return false;
        }
        return true;
    }
}}
namespace std {
    template<size_t N, typename T>
    struct hash<tdc::esp::Array<N, T>>
    {
        size_t operator()(const tdc::esp::Array<N, T>& x) const {
            return std::hash<tdc::ConstGenericView<T>>()(
                tdc::ConstGenericView<T> {
                    x.m_data.data(),
                    x.m_data.size()
                });
        }
    };
}
