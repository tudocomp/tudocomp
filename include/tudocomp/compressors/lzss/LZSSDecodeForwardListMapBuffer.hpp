#ifndef _INCLUDED_DECODE_FORWARD_LM_BUFFER_HPP_
#define _INCLUDED_DECODE_FORWARD_LM_BUFFER_HPP_

#include <map>
#include <sdsl/int_vector.hpp>
#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class DecodeForwardListMapBuffer {

private:
    std::vector<uliteral_t> m_buffer;
    std::unordered_map<len_t, std::vector<len_t>> m_fwd;
    sdsl::bit_vector m_decoded;

    len_t m_cursor;

    inline void decode_literal_at(len_t pos, uliteral_t c) {
        m_buffer[pos] = c;
        m_decoded[pos] = 1;

        auto it = m_fwd.find(pos);
        if(it != m_fwd.end()) {
            for(auto fwd : it->second) {
                decode_literal_at(fwd, c); // recursion
            }

            m_fwd.erase(pos);
        }
    }

public:
    inline DecodeForwardListMapBuffer(len_t size) : m_cursor(0) {
        m_buffer.resize(size, 0);
        m_decoded = sdsl::bit_vector(size, 0);
    }

    inline void decode_literal(uliteral_t c) {
        decode_literal_at(m_cursor++, c);
    }

    inline void decode_factor(len_t pos, len_t num) {
        for(len_t i = 0; i < num; i++) {
            len_t src = pos+i;
            if(m_decoded[src]) {
                decode_literal_at(m_cursor, m_buffer[src]);
            } else {
                m_fwd[src].push_back(m_cursor);
            }

            ++m_cursor;
        }
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

#endif