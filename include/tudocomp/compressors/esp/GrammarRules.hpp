#pragma once

#include <tudocomp/compressors/esp/HashArray.hpp>

namespace tdc {namespace esp {
    template<typename Pred, typename Update>
    struct Updater {
        Pred pred;
        Update update;
    };
    template<typename Pred, typename Update>
    inline Updater<Pred, Update> make_updater(Pred pred, Update update) {
        return Updater<Pred, Update> {
            pred,
            update,
        };
    }

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

            auto updater = make_updater(
                [](size_t v) { return v == 0; },
                [](size_t v) { return v + 1; }
            );

            size_t* r;
            if (vs == 2) {
                r = &n2.access(v, counter + 1);
            } else {

                Array<2> between;
                between.m_data[0] = add(v.slice(0, 2));
                between.m_data[1] = v[2];

                r = &n2.access(between, counter + 1);
            }

            if (*r == 0) {
                *r = counter++;
            }

            return *r - 1;
        }

        inline size_t rules_count() const {
            return counter - m_initial_counter;
        }

        inline size_t initial_counter() const {
            return m_initial_counter;
        }

        template<typename F>
        void for_all(F f) {
            n2.for_all(f);
        }

        inline void clear() {
            auto discard = std::move(n2);
        }
    };
}}
