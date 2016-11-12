#ifndef _INCLUDED_LZSS_LITERALS_HPP_
#define _INCLUDED_LZSS_LITERALS_HPP_

#include <tudocomp/Literal.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

namespace tdc {
namespace lzss {

template<typename text_t>
class Literals {
private:
    class iterator {
    private:
        const text_t* m_text;
        const lzss::FactorBuffer* m_factors;
        size_t m_pos;
        size_t m_next_factor;

        inline void skip_factors() {
            while(m_next_factor < m_factors->size() && m_pos == (*m_factors)[m_next_factor].pos) {
                m_pos += (*m_factors)[m_next_factor++].len;
            }
        }

    public:
        inline iterator(const text_t& text, const lzss::FactorBuffer& factors, size_t pos)
            : m_text(&text), m_factors(&factors), m_pos(pos), m_next_factor(0) {

            skip_factors();
        }

        inline Literal operator*() const {
            assert(m_pos < m_text->size());
            return {(*m_text)[m_pos], m_pos};
        }

        inline bool operator!= (const iterator& other) const {
            return (m_text != other.m_text || m_pos != other.m_pos);
        }

        inline iterator& operator++() {
            assert(m_pos < m_text->size());

            m_pos++;
            skip_factors();
            return *this;
        }
    };

    const text_t* m_text;
    const FactorBuffer* m_factors;

public:
    inline Literals(const text_t& text, const FactorBuffer& factors)
        : m_text(&text), m_factors(&factors) {
    }

    inline iterator begin() const {
        return iterator(*m_text, *m_factors, 0);
    }

    inline iterator end() const {
        return iterator(*m_text, *m_factors, m_text->size());
    }
};

}} //ns

#endif
