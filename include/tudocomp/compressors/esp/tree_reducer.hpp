#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/pre_header.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>

namespace tdc {
namespace esp {
    using in_t = ConstGenericView<size_t>;

    template<size_t N>
    struct Array {
        std::array<size_t, N> m_data;
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
}
}
namespace std {
    template<size_t N>
    struct hash<tdc::esp::Array<N>>
    {
        size_t operator()(const tdc::esp::Array<N>& x) const {
            return std::hash<ConstGenericView<size_t>>()(
                ConstGenericView<size_t> {
                    x.m_data.data(),
                    x.m_data.size()
                });
        }
    };
}

namespace tdc {
namespace esp {

    struct GrammarRules {
        GrammarRules() {}
        using a2_t = std::unordered_map<Array<2>, size_t>;
        using a3_t = std::unordered_map<Array<3>, size_t>;

        a2_t n2;
        a3_t n3;

        size_t counter = 0;

        void add(in_t v) {
            if (v.size() == 2) {
                n2[v] = counter++;
            } else if (v.size() == 3) {
                n3[v] = counter++;
            } else {
                DCHECK(false);
            }
        }
    };

    struct TreeReducer {

        void reduce(string_ref input) {
            GrammarRules gr;

            std::vector<size_t> layer;
            layer.reserve(input.size());
            for (auto c : input) {
                layer.push_back(c);
            }

            size_t alphabet = 256;

            std::vector<size_t> new_layer;

            in_t in = layer;

            size_t n = 0;
            std::cout << "[Round " << n << "]:\n\n";

            {
                esp::Context<in_t> ctx {
                    alphabet,
                    in,
                };
                ctx.print_mb2_trace = false;
                ctx.print_only_adjusted = true;

                esp::split(in, ctx);
                auto& v = ctx.adjusted_blocks();
                // v contains lengths of all blocks.
            }

        }
    };


}
}
