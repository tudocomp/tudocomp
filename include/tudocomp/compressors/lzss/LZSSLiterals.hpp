#ifndef _INCLUDED_LZSS_LITERALS_HPP_
#define _INCLUDED_LZSS_LITERALS_HPP_

#include <istream>
#include <tudocomp/Literal.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

namespace tdc {
namespace lzss {

template<typename text_t>
class TextLiterals {
private:
    const text_t* m_text;
    const FactorBuffer* m_factors;
    size_t m_pos;
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

        Literal l = {(*m_text)[m_pos], m_pos};

        ++m_pos; skip_factors();
        return l;
    }
};

class StreamLiterals {
private:
    std::istream* m_stream;
    const FactorBuffer* m_factors;
    size_t m_next_factor;

    inline void skip_factors() {
        while(!m_stream->eof() &&
            m_next_factor < m_factors->size() &&
            size_t(m_stream->tellg()) == (*m_factors)[m_next_factor].pos) {

            m_stream->seekg(
                (*m_factors)[m_next_factor++].len,
                std::ios_base::cur); // skip len characters
        }
    }

public:
    inline StreamLiterals(std::istream& stream, const FactorBuffer& factors)
        : m_stream(&stream), m_factors(&factors) {

        skip_factors();
    }

    inline bool has_next() const {
        return !m_stream->eof();
    }

    inline Literal next() {
        assert(has_next());

        size_t pos = m_stream->tellg();
        uint8_t c = m_stream->get();
        return Literal{ c, pos };
    }
};

}} //ns

#endif
