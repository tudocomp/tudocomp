#pragma once

#include <vector>
#include <tudocomp/def.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/lcpcomp/lcpcomp.hpp>

namespace tdc {
namespace lcpcomp {

constexpr len_t undef_len = std::numeric_limits<len_compact_t>::max();
class DecodeForwardQueueListBuffer : public Algorithm {
    public:
    inline static Meta meta() {
        Meta m(dec_strategy_type(), "QueueListBuffer");
        return m;
    }

private:
    std::vector<uliteral_t> m_buffer;
    std::vector<std::vector<len_compact_t>> m_fwd;
    BitVector m_decoded;

    len_t m_cursor;

    //stats:
    len_t m_longest_chain;
    len_t m_current_chain;
    len_t m_max_depth;

    inline void decode_literal_at(len_t pos, uliteral_t c) {
        ++m_current_chain;
        m_longest_chain = std::max(m_longest_chain, m_current_chain);
        m_max_depth = std::max<len_t>(m_max_depth, m_fwd[pos].size());

        m_buffer[pos] = c;
        m_decoded[pos] = 1;

        for(auto fwd : m_fwd[pos]) {
            decode_literal_at(fwd, c); // recursion
        }
        std::vector<len_compact_t>().swap(m_fwd[pos]); // forces vector to drop to capacity 0
//        m_fwd[pos].clear();

        --m_current_chain;
    }

public:
    inline DecodeForwardQueueListBuffer(Config&& cfg)
        : Algorithm(std::move(cfg)),
          m_cursor(0),
          m_longest_chain(0),
          m_current_chain(0),
          m_max_depth(0) {
    }

    inline void initialize(size_t n) {
        if(tdc_unlikely(n == 0)) throw std::runtime_error(
            "no text length provided");

        m_buffer.resize(n, 0);
        m_fwd.resize(n, std::vector<len_compact_t>());
        m_decoded = BitVector(n, 0);
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

    inline void process() {
    }

    inline len_t longest_chain() const {
        return m_longest_chain;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

