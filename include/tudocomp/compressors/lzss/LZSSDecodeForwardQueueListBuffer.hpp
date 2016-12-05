#ifndef _INCLUDED_DECODE_FORWARD_QL_BUFFER_HPP_
#define _INCLUDED_DECODE_FORWARD_QL_BUFFER_HPP_

#include <vector>
#include <sdsl/int_vector.hpp>
#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class DecodeForwardQueueListBuffer {

private:
    std::vector<uliteral_t> m_buffer;
    std::vector<std::vector<len_t>> m_fwd;
    sdsl::bit_vector m_decoded;

    len_t m_cursor;
    len_t m_longest_chain;
    len_t m_current_chain;

    inline void decode_literal_at(len_t pos, uliteral_t c) {
        ++m_current_chain;
        m_longest_chain = std::max(m_longest_chain, m_current_chain);

        m_buffer[pos] = c;
        m_decoded[pos] = 1;

        for(auto fwd : m_fwd[pos]) {
            decode_literal_at(fwd, c); // recursion
        }
        m_fwd[pos].clear();

        --m_current_chain;
    }

public:
    inline DecodeForwardQueueListBuffer(len_t size)
        : m_cursor(0), m_longest_chain(0), m_current_chain(0) {

        m_buffer.resize(size, 0);
        m_fwd.resize(size, std::vector<len_t>());
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

    inline len_t longest_chain() const {
        return m_longest_chain;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

#endif
