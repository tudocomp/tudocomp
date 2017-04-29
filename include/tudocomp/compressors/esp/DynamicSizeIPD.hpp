#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/HashMap.h>
#include <tudocomp/compressors/esp/HashArray.hpp>

namespace tdc {namespace esp {
    template<typename T>
    struct SizeAdjust {
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

        template<size_t N, typename T, typename U>
        class IPDMap {
            struct DynamicMap {
                using Updater = std::function<void(U&)>;
                using ForEach = std::function<void(const typename Array<N, T>::in_t&, const U&)>;

                virtual ~DynamicMap() {}

                virtual U access(const Array<N, T>& key, Updater updater) = 0;
                virtual size_t size() const = 0;
                virtual void for_all(ForEach f) const = 0;
            };
            template<size_t M, size_t O>
            struct DynamicMapOf: DynamicMap {
                using MappedT = typename SizeAdjust<T>::template Type<M>;
                using MappedU = typename SizeAdjust<U>::template Type<O>;

                typename ipd_t::template IPDMap<Array<N, MappedT>, MappedU> m_map;
            };

            std::unordered_map<Array<N, T>, U> m_map;

        public:
            inline IPDMap(size_t bucket_count, const Array<N, T>& empty)
                {}

            template<typename Updater>
            inline U access(const Array<N, T>& key, Updater updater) {
                auto& val = m_map[key];

                U val2 = val;
                updater(val2);
                val = val2;
                return val2;
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
