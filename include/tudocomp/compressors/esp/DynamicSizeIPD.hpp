#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/HashMap.h>
#include <tudocomp/compressors/esp/HashArray.hpp>

namespace tdc {namespace esp {
    template<typename T>
    struct SizeAdjust {
    };

    template<size_t N>
    struct SizeAdjust<Array<N, size_t>> {
        template<size_t M>
        using Type = Array<N, uint_t<M>>;
    };

    template<>
    struct SizeAdjust<size_t> {
        template<size_t N>
        using Type = uint_t<N>;
    };

    template<typename ipd_t>
    class DynamicSizeIPD: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("ipd", "dynamic_size");
            m.option("ipd").templated<ipd_t>("ipd");
            return m;
        };

        using Algorithm::Algorithm;

        template<typename T, typename U>
        class Map {
            struct DynamicMap {
                virtual ~DynamicMap() {}
            };
            template<size_t N>
            struct DynamicMapOf: DynamicMap {
                using MappedT = typename SizeAdjust<T>::template Type<N>;
                using MappedU = typename SizeAdjust<U>::template Type<N>;

                typename ipd_t::template Map<MappedT, MappedU> m_map;
            };

            std::unordered_map<T, U> m_map;

        public:
            inline Map(size_t bucket_count, const T& empty)
                {}

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
