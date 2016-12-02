#ifndef _INCLUDED_DECODE_BACK_BUFFER_HPP_
#define _INCLUDED_DECODE_BACK_BUFFER_HPP_

#include <ostream>
#include <vector>
#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class DecodeBackBuffer {

private:
    std::vector<uliteral_t> m_buffer;
    len_t m_cursor;

public:
    inline DecodeBackBuffer(len_t size) : m_cursor(0) {
        m_buffer.resize(size, 0);
    }

    inline void decode_literal(uliteral_t c) {
        m_buffer[m_cursor++] = c;
    }

    inline void decode_factor(len_t pos, len_t num) {
        while(num--) m_buffer[m_cursor++] = m_buffer[pos++];
    }

    inline len_t longest_chain() const {
        return 0;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

#endif
