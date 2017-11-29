#pragma once

#include <memory>
#include <array>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {namespace esp {

    static_assert(sizeof(std::array<size_t, 2>) == sizeof(size_t) * 2, "Something is not right");

    constexpr size_t SLP_CODING_ALPHABET_SIZE { 256 };

    class SLP {
        // TODO: Optimization: Store both l and r rules in m_dl if bit_width <= 32
        // for better cache locality
        DynamicIntVector m_dl;
        DynamicIntVector m_dr;
        size_t m_alphabet_size = 0;
        size_t m_root_rule = 0;
        bool m_is_empty = true;
    public:
        inline size_t width() const {
            return m_dl.width();
        }

        inline SLP(size_t alphabet_size /*, size_t width = 64*/):
            m_alphabet_size(alphabet_size)
        {
            resize(m_alphabet_size /*, width*/);
        }

        inline void resize(size_t size /*, size_t width*/) {
            // TODO: Fix bug in BitVector that makes setting width here not work
            size_t width = this->width();

            m_dl.reserve(size /*, width*/);
            m_dl.resize(size /*, width*/);

            m_dr.reserve(size /*, width*/);
            m_dr.resize(size /*, width*/);

            DCHECK_EQ(m_dl.width(), width);
            DCHECK_EQ(m_dr.width(), width);
        }

        /*
        inline void resize(size_t size) {
            resize(size, width());
        }
        */

        inline size_t get_l(size_t rule) const {
            return m_dl[rule];
        }

        inline size_t get_r(size_t rule) const {
            return m_dr[rule];
        }

        inline void set_l(size_t rule, size_t l) {
            m_dl[rule] = l;
        }

        inline void set_r(size_t rule, size_t r) {
            m_dr[rule] = r;
        }

        inline void set(size_t rule, size_t l, size_t r) {
            m_dl[rule] = l;
            m_dr[rule] = r;
        }

        inline void set_root_rule(size_t rule) {
            m_root_rule = rule;
        }

        inline void set_empty(bool empty) {
            m_is_empty = empty;
        }

        inline bool is_empty() const {
            return m_is_empty;
        }

        inline size_t root_rule() const {
            return m_root_rule;
        }

        template<typename F>
        inline void derive(F f, size_t rule) const {
            if (rule < m_alphabet_size) {
                f(rule);
            } else {
                derive(f, get_l(rule));
                derive(f, get_r(rule));
            }
        }

        template<typename F>
        inline void derive(F f) const {
            if (!m_is_empty) {
                derive(f, m_root_rule);
            }
        }

        inline std::ostream& derive_text(std::ostream& o) const {
            derive([&](size_t symbol) {
                o << char(symbol);
            });
            return o;
        }

        inline std::string derive_text_s() const {
            std::stringstream ss;
            derive_text(ss);
            return ss.str();
        }

        inline size_t size() const {
            DCHECK_EQ(m_dl.size(), m_dr.size());
            return m_dl.size();
        }

        inline size_t alphabet_size() const {
            return m_alphabet_size;
        }

        DynamicIntVector& dl() {
            return m_dl;
        }

        DynamicIntVector& dr() {
            return m_dr;
        }
    };

    /*
    /// Adapter to make the right symbols (_ -> _X) of
    /// the SLP accessible as a array.
    /// The first alphabet_size symbols will be hidden.
    class SLPRhsAdapter {
        SLP* m_slp;
    public:
        using value_type = size_t;

        class Accessor {
            SLP* m_slp;
            size_t m_i;
        public:
            Accessor(SLP& slp, size_t i): m_slp(&slp), m_i(i) {}

            inline void operator=(size_t val) {
                m_slp->set_r(m_i, val);
            }

            inline operator size_t() const {
                return m_slp->get_r(m_i);
            }
        };

        inline static void swap(Accessor& a, Accessor& b) {
            size_t tmp = a;
            a = b;
            b = tmp;
        }

        SLPRhsAdapter(SLP& slp): m_slp(&slp) {}

        inline const Accessor operator[](size_t i) const {
            return Accessor(*m_slp, i);
        }
        inline Accessor operator[](size_t i) {
            return Accessor(*m_slp, i);
        }
        inline size_t size() const {
            return m_slp->size();
        }
    };
    */
}}
