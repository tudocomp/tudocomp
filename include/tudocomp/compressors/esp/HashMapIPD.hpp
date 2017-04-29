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

        template<typename T, typename U>
        class Map {
            rigtorp::HashMap<T, U> m_map;

        public:
            inline Map(size_t bucket_count, const T& empty):
                m_map(bucket_count, empty) {}

            inline U& access(const T& key, size_t max_val) {
                return m_map[key];
            }

            inline auto begin() const {
                return m_map.begin();
            }

            inline auto end() const {
                return m_map.end();
            }

            inline size_t size() const {
                return m_map.size();
            }
        };
    };
}}
