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
    class text_end_iterator{};

    class text_iterator {
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
        inline text_iterator(const text_t& text, const lzss::FactorBuffer& factors)
            : m_text(&text), m_factors(&factors), m_pos(0), m_next_factor(0) {

            skip_factors();
        }

        inline Literal operator*() const {
            assert(m_pos < m_text->size());
            return {(*m_text)[m_pos], m_pos};
        }

        inline bool operator!= (const text_end_iterator& other) const {
            return m_pos >= m_text->size();
        }

        inline text_iterator& operator++() {
            assert(m_pos < m_text->size());

            m_pos++;
            skip_factors();
            return *this;
        }
    };

    const text_t* m_text;
    const FactorBuffer* m_factors;

public:
    inline TextLiterals(const text_t& text, const FactorBuffer& factors)
        : m_text(&text), m_factors(&factors) {
    }

    inline text_iterator begin() const {
        return text_iterator(*m_text, *m_factors);
    }

    inline text_end_iterator end() const {
        return text_end_iterator();
    }
	bool empty() const { return m_factors->empty(); }
};

class StreamLiterals {
private:
    class stream_end_iterator{};

    class stream_iterator {
    private:
        std::istream* m_stream;

        const lzss::FactorBuffer* m_factors;
        size_t m_next_factor;

        inline void skip_factors() {
            while(m_next_factor < m_factors->size() && size_t(m_stream->tellg()) == (*m_factors)[m_next_factor].pos) {
                m_stream->seekg(
                    (*m_factors)[m_next_factor++].len,
                    std::ios_base::cur); // skip len characters
            }
        }

    public:
        inline stream_iterator(std::istream& stream, const lzss::FactorBuffer& factors)
            : m_stream(&stream), m_factors(&factors), m_next_factor(0) {

            skip_factors();
        }

        inline Literal operator*() const {
            return Literal { uint8_t(m_stream->peek()), size_t(m_stream->tellg()) };
        }

        inline bool operator!= (const stream_end_iterator& other) const {
            return m_stream->eof();
        }

        inline stream_iterator& operator++() {
            m_stream->get(); // read one byte
            skip_factors();
            return *this;
        }
    };


    std::istream* m_stream;
    const FactorBuffer* m_factors;

public:
    inline StreamLiterals(std::istream& stream, const FactorBuffer& factors)
        : m_stream(&stream), m_factors(&factors) {
    }

    inline stream_iterator begin() const {
        return stream_iterator(*m_stream, *m_factors);
    }

    inline stream_end_iterator end() const {
        return stream_end_iterator();
    }
};

}} //ns

#endif
