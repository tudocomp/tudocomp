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
            inline Map() {}

            inline U& access(const T& key) {
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
