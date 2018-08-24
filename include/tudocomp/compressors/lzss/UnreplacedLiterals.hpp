#pragma once

#include <istream>
#include <tudocomp/Literal.hpp>
#include <tudocomp/compressors/lzss/FactorBuffer.hpp>

namespace tdc {
namespace lzss {

template<typename text_t, typename factorbuffer_t>
class UnreplacedLiterals : LiteralIterator {
private:
    const text_t* m_text;
    const factorbuffer_t* m_factors;
    len_t m_pos;
    typename factorbuffer_t::const_iterator m_next_factor;

    inline void skip_factors() {
        while(
            m_next_factor != m_factors->end() &&
            m_pos == m_next_factor->pos) {

            m_pos += len_t(m_next_factor->len);
            ++m_next_factor;
        }
    }

public:
    inline UnreplacedLiterals(const text_t& text, const factorbuffer_t& factors)
        : m_text(&text),
          m_factors(&factors),
          m_pos(0),
          m_next_factor(m_factors->begin()) {

        skip_factors();
    }

    inline bool has_next() const {
        return m_pos < m_text->size();
    }

    inline Literal next() {
        assert(has_next());

        Literal l = {uliteral_t((*m_text)[m_pos]), len_t(m_pos)};

        ++m_pos; skip_factors();
        return l;
    }
};

}} //ns

