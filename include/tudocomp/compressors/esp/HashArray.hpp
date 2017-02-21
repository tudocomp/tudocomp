#pragma once

#include<array>

#include<tudocomp/util/View.hpp>

namespace tdc {namespace esp {
    using in_t = ConstGenericView<size_t>;

    template<size_t N>
    struct Array {
        std::array<size_t, N> m_data;
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
    };

    template<size_t N>
    bool operator==(const Array<N>& lhs, const Array<N>& rhs) {
        for(size_t i = 0; i < N; i++) {
            if (lhs.m_data[i] != rhs.m_data[i]) return false;
        }
        return true;
    }
}}
namespace std {
    template<size_t N>
    struct hash<tdc::esp::Array<N>>
    {
        size_t operator()(const tdc::esp::Array<N>& x) const {
            return std::hash<tdc::ConstGenericView<size_t>>()(
                tdc::ConstGenericView<size_t> {
                    x.m_data.data(),
                    x.m_data.size()
                });
        }
    };
}
