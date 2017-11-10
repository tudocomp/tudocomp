#pragma once

#include <tudocomp/compressors/esp/HashArray.hpp>

namespace tdc {namespace esp {
    struct IPDStats {
        size_t ext_size2_total = 0;

        size_t ext_size3_total = 0;
        size_t ext_size3_unique = 0;

        size_t int_size2_total = 0;
        size_t int_size2_unique = 0;
    };

    template<typename ipd_t>
    class GrammarRules {
    public:
        using Stats = IPDStats;
    private:
        static constexpr std::array<size_t, 2> default_key() {
            return {{ size_t(-1), size_t(-1) }};
        }

        using a2_t = typename ipd_t::template IPDMap<2, size_t, size_t>;

        a2_t n2;

        size_t counter = 1;

        size_t m_initial_counter = 1;

        Stats m_stats;
    public:
        GrammarRules(size_t counter_start):
            n2(0, Array<2>(default_key())),
            counter(counter_start + 1),
            m_initial_counter(counter_start + 1) {}

        inline size_t add(in_t v) {
            const size_t vs = v.size();

            DCHECK(vs == 2 || vs == 3);

            auto updater = [&](size_t& v) {
                if (v == 0) {
                    v = counter++;
                }
            };

            if (vs == 2) {
                Array<2> va;
                va.m_data[0] = v[0];
                va.m_data[1] = v[1];

                auto old_counter = counter;
                auto r = n2.access(va, updater) - 1;
                if (counter > old_counter) {
                    m_stats.int_size2_unique++;
                }
                m_stats.int_size2_total++;
                m_stats.ext_size2_total++;
                return r;
            } else {
                Array<2> between;
                between.m_data[0] = add(v.slice(0, 2));
                between.m_data[1] = v[2];

                auto old_counter = counter;
                auto r = n2.access(between, updater) - 1;
                if (counter > old_counter) {
                    m_stats.ext_size3_unique++;
                    m_stats.int_size2_unique++;
                }
                m_stats.ext_size2_total--; // compensate for internal 2 use
                m_stats.ext_size3_total++;
                m_stats.int_size2_total++;
                return r;
            }
        }

        inline size_t rules_count() const {
            return counter - m_initial_counter;
        }

        inline size_t initial_counter() const {
            return m_initial_counter;
        }

        template<typename F>
        void for_all(F f) const {
            n2.for_all(f);
        }

        inline void clear() {
            auto discard = std::move(n2);
        }

        inline const Stats& stats() {
            return m_stats;
        }
    };
}}
