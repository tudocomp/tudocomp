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

        template<typename T, typename U>
        class Map {
            std::unordered_map<T, U> m_map;

        public:
            inline Map(size_t bucket_count, const T& empty) {}

            template<typename Updater>
            inline U& access(const T& key, Updater updater) {
                auto& val = m_map[key];
                updater(val);
                return m_map[key];
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
