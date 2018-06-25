#pragma once

#include <tudocomp/def.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lcpcomp {

class MultimapBuffer : public Algorithm {
    public:
    inline static Meta meta() {
        Meta m("lcpcomp_dec", "MultimapListBuffer");
        m.option("lazy").dynamic(0);
        return m;
    }

private:
    std::vector<uliteral_t> m_buffer;
    std::unordered_multimap<len_compact_t, len_compact_t> m_fwd;
    BitVector m_decoded;

    len_t m_cursor;
    len_t m_longest_chain;
    len_t m_current_chain;

    //storing factors
    std::vector<len_compact_t> m_target_pos;
    std::vector<len_compact_t> m_source_pos;
    std::vector<len_compact_t> m_length;

    const size_t m_lazy; // number of lazy rounds

    inline void decode_literal_at(len_t pos, uliteral_t c) {
        ++m_current_chain;
        m_longest_chain = std::max(m_longest_chain, m_current_chain);

        m_buffer[pos] = c;
        m_decoded[pos] = 1;
        // {
// 		const auto range = m_fwd.equal_range(pos);
// 		for (auto it = range.first; it != range.second; ++it) {
//             decode_literal_at(it->second, c); // recursion
// 		}
// }
// 		m_fwd.erase(pos);
        //
        //
        //
        for(auto it = m_fwd.find(pos); it != m_fwd.end(); it = m_fwd.find(pos)) { //TODO: replace with equal_range!
            decode_literal_at(it->second, c); // recursion
            m_fwd.erase(it);
        }

        // for(auto it = m_fwd.find(pos); it != m_fwd.end(); it = m_fwd.find(pos)) { //TODO: replace with equal_range!
        //     decode_literal_at(it->second, c); // recursion
        // }
        // m_fwd.erase(pos);

        --m_current_chain;
    }

    inline void decode_lazy_() {
        const len_t factors = m_source_pos.size();
        for(len_t j = 0; j < factors; ++j) {
            const len_compact_t& target_position = m_target_pos[j];
            const len_compact_t& source_position = m_source_pos[j];
            const len_compact_t& factor_length = m_length[j];
            for(len_t i = 0; i < factor_length; ++i) {
                if(m_decoded[source_position+i]) {
                    m_buffer[target_position+i] = m_buffer[source_position+i];
                    m_decoded[target_position+i] = 1;
                }
            }
        }
    }

public:
    inline MultimapBuffer(Env&& env)
        : Algorithm(std::move(env)), m_cursor(0), m_longest_chain(0), m_current_chain(0), m_lazy(this->env().option("lazy").as_integer())
    {
		m_fwd.max_load_factor(0.8);
    }

    inline void initialize(size_t n) {
        if(tdc_unlikely(n == 0)) throw std::runtime_error(
            "no text length provided");

        m_buffer.resize(n, 0);
        m_decoded = BitVector(n, 0);
    }

    inline void decode_literal(uliteral_t c) {
        decode_literal_at(m_cursor++, c);
    }

    inline void decode_factor(const len_t source_position, const len_t factor_length) {
        bool factor_stored = false;
        for(len_t i = 0; i < factor_length; ++i) {
            const len_t src_pos = source_position+i;
            if(m_decoded[src_pos]) {
                m_buffer[m_cursor] = m_buffer[src_pos];
                m_decoded[m_cursor] = 1;
            }
            else if(factor_stored == false) {
                factor_stored = true;
                m_target_pos.push_back(m_cursor);
                m_source_pos.push_back(src_pos);
                m_length.push_back(factor_length-i);
            }
            ++m_cursor;
        }
    }

    inline void decode_lazy() {
        size_t lazy = m_lazy;
        while(lazy > 0) {
            decode_lazy_();
            --lazy;
        }
    }

    inline void decode_eagerly() {
        const len_t factors = m_source_pos.size();
		StatPhase phase("Decoding Factors");
        phase.log_stat("factors", factors);

        IF_STATS(size_t max_size = 0);

        for(len_t j = 0; j < factors; ++j) {
            const len_compact_t& target_position = m_target_pos[j];
            const len_compact_t& source_position = m_source_pos[j];
            const len_compact_t& factor_length = m_length[j];
            for(len_t i = 0; i < factor_length; ++i) {
                if(m_decoded[source_position+i]) {
                    decode_literal_at(target_position+i, m_buffer[source_position+i]);
                } else {
                    m_fwd.emplace(source_position+i, target_position+i);
                }
            }
			IF_STATS({
                max_size = std::max(max_size, m_fwd.bucket_count());
			    if((j+1) % ((factors+5)/5) == 0 ) {
				    phase.log_stat("hash table size", m_fwd.bucket_count());
				    phase.log_stat("hash table entries", m_fwd.size());

				    phase.split(
                        std::string("Decoding Factors at position ")
                         + std::to_string(target_position));
			    }
            })
        }
		IF_STATS(phase.log_stat("hash table max size", max_size));
    }

    inline void process() {
        decode_lazy();
        decode_eagerly();
    }

    inline len_t longest_chain() const {
        return m_longest_chain;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};


}}//ns

