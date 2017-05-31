#pragma once

#include <ostream>
#include <vector>
#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class DecodeBackBuffer{

private:
    std::vector<uliteral_t> m_buffer;
    index_fast_t m_cursor;

public:

    inline DecodeBackBuffer(index_fast_t size)
		: m_cursor(0)
    {
        m_buffer.resize(size, 0);
    }

    inline void decode_literal(uliteral_t c) {
        m_buffer[m_cursor++] = c;
    }

    inline void decode_factor(index_fast_t pos, index_fast_t num) {
        while(num--) m_buffer[m_cursor++] = m_buffer[pos++];
    }

    inline index_fast_t longest_chain() const {
        return 0;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

