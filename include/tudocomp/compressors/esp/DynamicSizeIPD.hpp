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

        template<size_t N>
        inline uint_t<N> cast_to(size_t val) {
            return val;
        }

        template<size_t N>
        inline size_t cast_from(uint_t<N> val) {
            return val;
        }
    };

    template<size_t M>
    struct SizeAdjust<Array<M, size_t>> {
        template<size_t N>
        using Type = Array<M, uint_t<N>>;

        template<size_t N>
        inline Array<M, uint_t<N>> cast_to(Array<M, size_t> val) {
            Array<M, uint_t<N>> ret;
            for(size_t i = 0; i < ret.size(); i++) {
                ret[i] = val[i];
            }

            return val;
        }

        template<size_t N>
        inline Array<M, size_t> cast_from(Array<M, uint_t<N>> val) {
            Array<M, size_t> ret;
            for(size_t i = 0; i < ret.size(); i++) {
                ret[i] = val[i];
            }

            return val;
        }
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
                virtual bool can_fit_value(U new_val) const = 0;
                virtual bool can_fit_key(const Array<N, T>& key) const = 0;
            };
            template<size_t M, size_t O>
            struct DynamicMapOf: DynamicMap {
                using Updater = typename DynamicMap::Updater;
                using ForEach = typename DynamicMap::ForEach;
                using MappedT = typename SizeAdjust<T>::template Type<M>;
                using MappedU = typename SizeAdjust<U>::template Type<O>;

                Array<N, MappedT> m_empty;
                typename ipd_t::template IPDMap<Array<N, MappedT>, MappedU> m_map;

                inline DynamicMapOf(size_t bucket_count, const Array<N, T>& empty):
                    m_empty(typename SizeAdjust<Array<N, T>>::cast_to(empty)),
                    m_map(bucket_count, m_empty) {}

                inline virtual size_t size() const override final {
                    return m_map.size();
                }

                inline virtual void for_all(ForEach f) const override final {
                    for(auto& kv : m_map) {
                        const auto& key = kv.first;
                        const auto& val = kv.second;

                        const auto& key2 = typename SizeAdjust<Array<N, T>>::cast_to(key);
                        const auto& val2 = typename SizeAdjust<T>::cast_to(val);

                        f(key.as_view(), val);
                    }
                }

                inline virtual bool can_fit_value(U val) const override final {
                    return bits_for(val) <= O;
                }

                inline virtual bool can_fit_key(const Array<N, T>& key) const override final {
                    if (typename SizeAdjust<Array<N, T>>::cast_to(key) == m_empty) {
                        return false;
                    }

                    size_t val = 0;
                    for (auto x : key.as_view()) {
                        val = std::max(val, x);
                    }
                    return bits_for(val) <= M;
                }

                inline virtual U access(const Array<N, T>& key, Updater updater) const override final {
                    Array<N, MappedT> actual_key = typename SizeAdjust<Array<N, T>>::cast_to(key);
                    return m_map.access(actual_key, [&](MappedU& val) {
                        U val2 = typename SizeAdjust<T>::cast_from(val);
                        updater(val2);
                        val = typename SizeAdjust<T>::cast_to(val2);
                    });
                }
            };

            std::unique_ptr<DynamicMap> m_map;

        public:
            inline IPDMap(size_t bucket_count, const Array<N, T>& empty) {
                m_map = std::make_unique<DynamicMapOf<64, 64>>(bucket_count, empty);
            }

            template<typename Updater>
            inline U access(const Array<N, T>& key, Updater updater) {
                if (!m_map->can_fit_key(key)) {
                    // RESIZE
                }

                auto copy = U();
                bool value_resize = false;

                U ret = m_map->access(key, [&](U& val) {
                    copy = val;
                    updater(copy);
                    if (m_map->can_fit_value(copy)) {
                        val = copy;
                    } else {
                        value_resize = true;
                    }
                });

                if (value_resize) {
                    // RESIZE
                    ret = m_map->access(key, [&](U& val) {
                        val = copy;
                    });
                }

                DCHECK_EQ(ret, copy);

                return copy;
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
