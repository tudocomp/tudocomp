#pragma once

#include <vector>
#include <sdsl/int_vector.hpp>
#include <tudocomp/def.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/compressors/lcpcomp/decoding/DecodeQueueListBuffer.hpp>

namespace tdc {
namespace lcpcomp {

class SuccinctListBuffer : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("lcpdec", "SuccinctListBuffer");
        return m;

    }
    inline void decode_lazy() const {
    }
    inline void decode_eagerly() const {
    }

private:
    len_t** m_fwd;

    len_t m_cursor;
    IF_STATS(len_t m_longest_chain);
    IF_STATS(len_t m_current_chain);

    IntVector<uliteral_t> m_buffer;

    inline void decode_literal_at(len_t pos, uliteral_t c) {
		IF_STATS(++m_current_chain);
        IF_STATS(m_longest_chain = std::max(m_longest_chain, m_current_chain));

        m_buffer[pos] = c;
		DCHECK(c != 0 || pos == m_buffer.size()-1); // we assume that the text to restore does not contain a NULL-byte but at its very end

        if(m_fwd[pos] != nullptr) {
            const len_t*const& bucket = m_fwd[pos];
            for(size_t i = 1; i < bucket[0]; ++i) {
                decode_literal_at(bucket[i], c); // recursion
            }
            delete [] m_fwd[pos];
            m_fwd[pos] = nullptr;
        }

        IF_STATS(--m_current_chain);
    }

public:
    SuccinctListBuffer(SuccinctListBuffer&& other):
        Algorithm(std::move(*this)),
        m_fwd(std::move(other.m_fwd)),
        m_cursor(std::move(other.m_cursor)),
        m_buffer(std::move(other.m_buffer))
    {
        IF_STATS(m_longest_chain = std::move(other.m_longest_chain));
        IF_STATS(m_current_chain = std::move(other.m_current_chain));

        other.m_fwd = nullptr;
    }

    ~SuccinctListBuffer() {
        if(m_fwd != nullptr) {
            for(size_t i = 0; i < m_buffer.size(); ++i) {
                if(m_fwd[i] == nullptr) continue;
                delete [] m_fwd[i];
            }
            delete [] m_fwd;
        }
    }
    inline SuccinctListBuffer(Env&& env, len_t size)
        : Algorithm(std::move(env)), m_cursor(0), m_buffer(size,0) {

        IF_STATS(m_longest_chain = 0);
        IF_STATS(m_current_chain = 0);

        m_fwd = new len_t*[size];
        std::fill(m_fwd,m_fwd+size,nullptr);
    }

    inline void decode_literal(uliteral_t c) {
        decode_literal_at(m_cursor++, c);
    }

    inline void decode_factor(len_t pos, len_t num) {
        for(len_t i = 0; i < num; i++) {
            len_t src = pos+i;
            if(m_buffer[src]) {
                decode_literal_at(m_cursor, m_buffer[src]);
            } else {
                len_t*& bucket = m_fwd[src];
                if(bucket == nullptr) {
                    bucket = new len_t[2];
                    DCHECK(m_fwd[src] == bucket);
                    bucket[0] = 2;
					bucket[1] = m_cursor;
                }
				else
                { // this block implements the call of m_fwd[src]->push_back(m_cursor);
					++bucket[0]; // increase the size of a bucket only by one!
					bucket = (len_t*) realloc(bucket, sizeof(len_t)*bucket[0]);
                    bucket[bucket[0]-1] = m_cursor;
                }
            }

            ++m_cursor;
        }
    }

    IF_STATS(
    inline len_t longest_chain() const {
        return m_longest_chain;
    })

    inline void write_to(std::ostream& out) const {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

