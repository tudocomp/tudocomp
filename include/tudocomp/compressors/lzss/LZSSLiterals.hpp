#pragma once

#include <istream>
#include <tudocomp/Literal.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

namespace tdc {
namespace lzss {

template<typename text_t>
class TextLiterals : LiteralIterator {
private:
    const text_t* m_text;
    const FactorBuffer* m_factors;
    len_t m_pos;
    size_t m_next_factor;

    inline void skip_factors() {
        while(m_next_factor < m_factors->size() && m_pos == (*m_factors)[m_next_factor].pos) {
            m_pos += (*m_factors)[m_next_factor++].len;
        }
    }

public:
    inline TextLiterals(const text_t& text, const FactorBuffer& factors)
        : m_text(&text), m_factors(&factors), m_pos(0), m_next_factor(0) {

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

