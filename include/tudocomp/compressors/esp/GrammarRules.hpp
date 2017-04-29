#pragma once

#include <tudocomp/compressors/esp/HashArray.hpp>

namespace tdc {namespace esp {
    template<typename ipd_t>
    class GrammarRules {
        static constexpr std::array<size_t, 2> default_key() {
            return {{ size_t(-1), size_t(-1) }};
        }

        using a2_t = typename ipd_t::template Map<Array<2>, size_t>;

        a2_t n2;

        size_t counter = 1;

        size_t m_initial_counter = 1;
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
                return n2.access(v, updater) - 1;
            } else {
                Array<2> between;
                between.m_data[0] = add(v.slice(0, 2));
                between.m_data[1] = v[2];

                return n2.access(between, updater) - 1;
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
    };
}}
