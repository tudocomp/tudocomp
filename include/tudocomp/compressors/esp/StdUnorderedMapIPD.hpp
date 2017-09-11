#pragma once

#include <tudocomp/Algorithm.hpp>

namespace tdc {namespace esp {
    class StdUnorderedMapIPD: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("ipd", "std_unordered_map");
            return m;
        };

        using Algorithm::Algorithm;

        template<size_t N, typename T, typename U>
        class IPDMap {
            std::unordered_map<Array<N, T>, U> m_map;

        public:
            inline IPDMap(size_t bucket_count, const Array<N, T>& empty) {}

            template<typename Updater>
            inline U access(const Array<N, T>& key, Updater updater) {
                auto& val = m_map[key];
                updater(val);
                return val;
            }

            inline size_t size() const {
                return m_map.size();
            }

            template<typename F>
            void for_all(F f) const {
                for(auto& kv : m_map) {
                    const auto& key = kv.first;
                    const auto& val = kv.second;
                    f(key, val);
                }
            }
        };
    };
}}
