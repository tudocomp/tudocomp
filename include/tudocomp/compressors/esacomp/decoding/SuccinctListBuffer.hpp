#pragma once

#include <vector>
#include <sdsl/int_vector.hpp>
#include <tudocomp/def.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/compressors/esacomp/decoding/DecodeQueueListBuffer.hpp>

namespace tdc {
namespace esacomp {

class SuccinctListBuffer : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("esadec", "SuccinctListBuffer");
        return m;

    }
    inline void decode_lazy() const {
    }
    inline void decode_eagerly() const {
    }

private:
    std::vector<uliteral_t> m_buffer;
    //typedef IntVector<dynamic_t> vector_t;
//	typedef std::vector<len_t> vector_t;
    len_t** m_fwd;
//    std::vector<std::vector<len_t>> m_fwd;
    sdsl::bit_vector m_decoded;

    len_t m_cursor;
    len_t m_longest_chain;
    len_t m_current_chain;

    const len_t m_size;

    inline void decode_literal_at(len_t pos, uliteral_t c) {
        ++m_current_chain;
        m_longest_chain = std::max(m_longest_chain, m_current_chain);

        m_buffer[pos] = c;
        m_decoded[pos] = 1;

        if(m_fwd[pos] != nullptr) {
            const len_t*const& bucket = m_fwd[pos];
            for(size_t i = 1; i < bucket[0]; ++i) {
                //len_t i = (dynamic_t)fwd;
                if(bucket[i] == undef_len) break;
                decode_literal_at(bucket[i], c); // recursion
            }
            delete [] m_fwd[pos];
            m_fwd[pos] = nullptr;
        }

        --m_current_chain;
    }

public:
    SuccinctListBuffer(SuccinctListBuffer&& other):
        Algorithm(std::move(*this)),
        m_buffer(std::move(other.m_buffer)),
        m_fwd(std::move(other.m_fwd)),
        m_cursor(std::move(other.m_cursor)),
        m_longest_chain(std::move(other.m_longest_chain)),
        m_current_chain(std::move(other.m_current_chain)),
        m_size(std::move(other.m_size))
    {
        other.m_fwd = nullptr;
    }

    ~SuccinctListBuffer() {
        if(m_fwd != nullptr) {
            for(size_t i = 0; i < m_size; ++i) {
                if(m_fwd[i] == nullptr) continue;
                delete [] m_fwd[i];
            }
            delete [] m_fwd;
        }
    }
    inline SuccinctListBuffer(Env&& env, len_t size)
        : Algorithm(std::move(env)), m_cursor(0), m_longest_chain(0), m_current_chain(0), m_size(size) {

        m_buffer.resize(size, 0);
        m_fwd = new len_t*[size];
        std::fill(m_fwd,m_fwd+size,nullptr);
//        m_fwd.resize(size, std::vector<len_t>());
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
                len_t*& bucket = m_fwd[src];
                if(bucket == nullptr) {
                    m_fwd[src] = new len_t[2];
                    DCHECK(m_fwd[src] == bucket);
                    std::fill(bucket,bucket+2, undef_len);
                    bucket[0] = 2;
                    //m_fwd[src] = new vector_t(0,0, bits_for(m_size));
                }
                { // m_fwd[src]->push_back(m_cursor);
                    len_t i = 1;
                    while(i < bucket[0]) {
                        if(bucket[i] == undef_len) break;
                        ++i;
                    }
                    if(i == bucket[0]) {
                        DCHECK(m_fwd[src] == bucket);
                        bucket[0] = ++m_fwd[src][0]; //(bucket[0]*16)/10+1;
                        bucket = (len_t*) realloc(bucket, sizeof(len_t)*bucket[0]);
                        DCHECK(m_fwd[src] == bucket);
                    }
                    bucket[i] = m_cursor;
                }
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

