#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/HashMap.h>

namespace tdc {namespace esp {
    class HashMapIPD: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("ipd", "hash_map");
            return m;
        };

        using Algorithm::Algorithm;

        template<size_t N, typename T, typename U>
        class IPDMap {
            rigtorp::HashMap<Array<N, T>, U> m_map;

        public:
            inline IPDMap(size_t bucket_count, const Array<N, T>& empty):
                m_map(bucket_count, empty) {}

            template<typename Updater>
            inline size_t access(const Array<N, T>& key, Updater updater) {
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
                    const auto& key = kv.first.as_view();
                    const auto& val = kv.second;
                    f(key, val);
                }
            }
        };
    };
}}
