#ifndef MARVINBUFFER_HPP
#define MARVINBUFFER_HPP

#include <vector>
#include <sdsl/int_vector.hpp>
#include <tudocomp/def.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {
namespace esacomp {

class MarvinBuffer : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("esadec", "Marvin");
        return m;

    }
    inline void decode_lazy() const {
    }
    inline void decode_eagerly() const {
    }

private:
    std::vector<uliteral_t> m_buffer;
    std::unique_ptr<std::unique_ptr<len_t[]>[]> m_fwd;
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
            const std::unique_ptr<len_t[]>& bucket = m_fwd[pos];
            for(size_t i = 1; i < bucket[0]; ++i) {
                //len_t i = (dynamic_t)fwd;
                if(bucket[i] == undef_len) break;
                decode_literal_at(bucket[i], c); // recursion
            }
            m_fwd[pos].reset();
        }

        --m_current_chain;
    }

public:
    inline MarvinBuffer(Env&& env, len_t size)
        : Algorithm(std::move(env)), m_cursor(0), m_longest_chain(0), m_current_chain(0), m_size(size) {

        m_buffer.resize(size, 0);
        m_fwd = std::make_unique<std::unique_ptr<len_t[]>[]>(size);
        std::fill(m_fwd.get(), m_fwd.get() + size,nullptr);
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
                std::unique_ptr<len_t[]>& bucket = m_fwd[src];
                if(bucket == nullptr) {
                    m_fwd[src] = std::make_unique<len_t[]>(2);
                    DCHECK(m_fwd[src] == bucket);
                    std::fill(bucket.get(), bucket.get() + 2, undef_len);
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

                        // It is incorrect to use realloc with new/delete
                        // allocated pointers
                        // https://isocpp.org/wiki/faq/freestore-mgmt#realloc-and-renew
                        //
                        // We will just assume it works though...

                        len_t* p = bucket.release();
                        auto new_size = sizeof(len_t) * bucket[0];
                        p = (len_t*) std::realloc(p, new_size);
                        bucket.reset(p);

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

#endif /* MARVINBUFFER_HPP */
