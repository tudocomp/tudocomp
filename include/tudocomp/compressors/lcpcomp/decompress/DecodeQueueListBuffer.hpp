#pragma once

#include <vector>
#include <tudocomp/def.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {
namespace lcpcomp {

constexpr index_fast_t undef_len = std::numeric_limits<index_t>::max();
class DecodeForwardQueueListBuffer : public Algorithm {
    public:
    inline static Meta meta() {
        Meta m("lcpcomp_dec", "QueueListBuffer");
        return m;
    }
    inline void decode_lazy() const {
    }
    inline void decode_eagerly() const {
    }

private:
    std::vector<uliteral_t> m_buffer;
    std::vector<std::vector<index_t>> m_fwd;
    BitVector m_decoded;

    index_fast_t m_cursor;

    //stats:
    index_fast_t m_longest_chain;
    index_fast_t m_current_chain;
    index_fast_t m_max_depth;

    inline void decode_literal_at(index_fast_t pos, uliteral_t c) {
        ++m_current_chain;
        m_longest_chain = std::max(m_longest_chain, m_current_chain);
        m_max_depth = std::max<index_fast_t>(m_max_depth, m_fwd[pos].size());

        m_buffer[pos] = c;
        m_decoded[pos] = 1;

        for(auto fwd : m_fwd[pos]) {
            decode_literal_at(fwd, c); // recursion
        }
        std::vector<index_t>().swap(m_fwd[pos]); // forces vector to drop to capacity 0
//        m_fwd[pos].clear();

        --m_current_chain;
    }

public:
    inline DecodeForwardQueueListBuffer(Env&& env, index_fast_t size)
        : Algorithm(std::move(env)), m_cursor(0), m_longest_chain(0), m_current_chain(0), m_max_depth(0) {

        m_buffer.resize(size, 0);
        m_fwd.resize(size, std::vector<index_t>());
        m_decoded = BitVector(size, 0);
    }

    inline void decode_literal(uliteral_t c) {
        decode_literal_at(m_cursor++, c);
    }

    inline void decode_factor(index_fast_t pos, index_fast_t num) {
        for(index_fast_t i = 0; i < num; i++) {
            index_fast_t src = pos+i;
            if(m_decoded[src]) {
                decode_literal_at(m_cursor, m_buffer[src]);
            } else {
                m_fwd[src].push_back(m_cursor);
            }

            ++m_cursor;
        }
    }

    inline index_fast_t longest_chain() const {
        return m_longest_chain;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

