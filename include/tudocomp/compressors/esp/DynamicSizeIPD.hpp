#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/HashMap.h>
#include <tudocomp/compressors/esp/HashArray.hpp>

namespace tdc {namespace esp {
    template<typename ipd_t>
    class DynamicSizeIPD: public Algorithm {
        template<typename X, size_t B>
        struct SizeAdjust {
        };

        template<size_t B>
        struct SizeAdjust<size_t, B> {
            using Type = uint_t<B>;

            inline static uint_t<B> cast_to(size_t val) {
                return val;
            }

            inline static size_t cast_from(uint_t<B> val) {
                return val;
            }
        };

        template<size_t N, size_t B>
        struct SizeAdjust<Array<N, size_t>, B> {
            using Type = Array<N, uint_t<B>>;

            inline static Array<N, uint_t<B>> cast_to(Array<N, size_t> val) {
                Array<N, uint_t<B>> ret;
                for(size_t i = 0; i < ret.m_data.size(); i++) {
                    ret.m_data[i] = val.m_data[i];
                }

                return ret;
            }

            inline static Array<N, size_t> cast_from(Array<N, uint_t<B>> val) {
                Array<N, size_t> ret;
                for(size_t i = 0; i < ret.m_data.size(); i++) {
                    ret.m_data[i] = val.m_data[i];
                }

                return ret;
            }
        };
    public:
        inline static Meta meta() {
            Meta m(TypeDesc("ipd"), "dynamic_size");
            m.param("ipd").strategy<ipd_t>(TypeDesc("ipd"));
            return m;
        };

        using Algorithm::Algorithm;

        template<size_t N, typename K, typename V>
        class IPDMap {
            inline static K key_max(const Array<N, K>& ka) {
                K val = 0;
                for (auto x : ka.as_view()) {
                    val = std::max(val, x);
                }
                return val;
            }

            struct DynamicMap {
                using Updater = std::function<void(V&)>;
                using ForEach = std::function<void(const Array<N, K>&, const V&)>;

                virtual ~DynamicMap() {}

                virtual V access(const Array<N, K>& key, Updater updater) = 0;
                virtual size_t size() const = 0;
                virtual void for_all(ForEach f) const = 0;
                virtual bool can_fit_value(V new_val) const = 0;
                virtual bool can_fit_key(const Array<N, K>& key) const = 0;
            };
            template<size_t BK, size_t BV>
            struct DynamicMapOf: DynamicMap {
                using Updater = typename DynamicMap::Updater;
                using ForEach = typename DynamicMap::ForEach;
                using MappedK = typename SizeAdjust<K, BK>::Type;
                using MappedV = typename SizeAdjust<V, BV>::Type;

                Array<N, MappedK> m_empty;
                typename ipd_t::template IPDMap<N, MappedK, MappedV> m_map;

                inline DynamicMapOf(size_t bucket_count, const Array<N, K>& empty):
                    m_empty(SizeAdjust<Array<N, K>, BK>::cast_to(empty)),
                    m_map(bucket_count, m_empty) {}

                inline virtual size_t size() const override final {
                    return m_map.size();
                }

                inline virtual void for_all(ForEach f) const override final {
                    m_map.for_all([&](const auto& key, const auto& val) {
                        const auto& key2 = SizeAdjust<Array<N, K>, BK>::cast_from(key);
                        const auto& val2 = SizeAdjust<V, BK>::cast_from(val);

                        f(key2.as_view(), val2);
                    });
                }

                inline virtual bool can_fit_value(V val) const override final {
                    return bits_for(val) <= BV;
                }

                inline virtual bool can_fit_key(const Array<N, K>& key) const override final {
                    if (SizeAdjust<Array<N, K>, BK>::cast_to(key) == m_empty) {
                        return false;
                    }

                    size_t val = key_max(key);
                    return bits_for(val) <= BK;
                }

                inline virtual V access(const Array<N, K>& key, Updater updater) override final {
                    auto actual_key = SizeAdjust<Array<N, K>, BK>::cast_to(key);
                    return m_map.access(actual_key, [&](MappedV& val) {
                        V val2 = SizeAdjust<V, BV>::cast_from(val);
                        updater(val2);
                        val = SizeAdjust<V, BV>::cast_to(val2);
                    });
                }
            };

            Array<N, K> m_empty;
            size_t m_map_bits = 0;
            std::unique_ptr<DynamicMap> m_map;

            const static size_t INITIAL_BITS = 8;

            template<size_t B>
            std::unique_ptr<DynamicMap> new_map(size_t bucket_count, const Array<N, K>& empty) {
                return std::make_unique<DynamicMapOf<B, B>>(bucket_count, empty);
            }

            inline void resize(size_t bits) {
                std::unique_ptr<DynamicMap> n_map;
                size_t n_bits = 0;

                // TODO: Calc bucket count correctly
                size_t s = m_map->size();

                if (false) {}
                else if (bits <= 8)  { n_map = new_map<8> (s, m_empty); n_bits = 8;  }
                else if (bits <= 16) { n_map = new_map<16>(s, m_empty); n_bits = 16; }
                else if (bits <= 24) { n_map = new_map<24>(s, m_empty); n_bits = 24; }
                else if (bits <= 32) { n_map = new_map<32>(s, m_empty); n_bits = 32; }
                else if (bits <= 40) { n_map = new_map<40>(s, m_empty); n_bits = 40; }
                else if (bits <= 48) { n_map = new_map<48>(s, m_empty); n_bits = 48; }
                else if (bits <= 56) { n_map = new_map<56>(s, m_empty); n_bits = 56; }
                else if (bits <= 64) { n_map = new_map<64>(s, m_empty); n_bits = 64; }

                m_map->for_all([&](const auto& key, const auto& val) {
                    n_map->access(key, [&](auto& v) {
                        v = val;
                    });
                });

                DCHECK_EQ(m_map->size(), n_map->size());
                m_map = std::move(n_map);
                m_map_bits = n_bits;
            }

            inline void resize_key(const Array<N, K>& max_key) {
                auto max = key_max(max_key);
                resize(std::max(size_t(bits_for(max)), m_map_bits + 1));
            }
            inline void resize_val(V max_val) {
                auto max = max_val;
                resize(std::max(size_t(bits_for(max)), m_map_bits + 1));
            }
        public:
            inline IPDMap(size_t bucket_count, const Array<N, K>& empty) {
                m_empty = empty;
                m_map_bits = INITIAL_BITS;
                m_map = new_map<INITIAL_BITS>(bucket_count, m_empty);
            }

            template<typename Updater>
            inline V access(const Array<N, K>& key, Updater updater) {
                if (!m_map->can_fit_key(key)) {
                    resize_key(key);
                }

                auto copy = V();
                bool value_resize = false;

                V ret = m_map->access(key, [&](V& val) {
                    copy = val;
                    updater(copy);
                    if (m_map->can_fit_value(copy)) {
                        val = copy;
                    } else {
                        value_resize = true;
                    }
                });

                if (value_resize) {
                    resize_val(copy);
                    ret = m_map->access(key, [&](V& val) {
                        val = copy;
                    });
                }

                DCHECK_EQ(ret, copy);

                return copy;
            }

            inline size_t size() const {
                return m_map->size();
            }

            template<typename F>
            void for_all(F f) const {
                m_map->for_all(f);
            }
        };
    };
}}
