#pragma once

#include <vector>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support.hpp>
#include <tudocomp/def.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/compressors/esacomp/decoding/DecodeQueueListBuffer.hpp>
#include <algorithm>


namespace tdc {
namespace esacomp {


	class LazyDecoder {
		Env& m_env;
		IntVector<uliteral_t>& m_buffer;
		const sdsl::bit_vector m_bv;
		const sdsl::bit_vector::rank_1_type m_rank;
		const len_t m_empty_entries;
		len_t**const m_fwd = nullptr;

		tdc_stats(len_t m_longest_chain = 0);
		tdc_stats(len_t m_current_chain = 0);


		public:
		LazyDecoder(Env& env, IntVector<uliteral_t>& buffer)
			: m_env(env)
			, m_buffer { buffer }
			, m_bv ( [&buffer] () -> sdsl::bit_vector {
				sdsl::bit_vector bv { buffer.size(),0 };
				for(len_t i = 0; i < buffer.size(); ++i) {
					if(buffer[i]) continue;
					bv[i] = 1;
				}
				return bv;
			}() )
			, m_rank { &m_bv }
			//, m_empty_entries { static_cast<len_t>( buffer.size()) }
			, m_empty_entries { static_cast<len_t>(std::count_if(buffer.cbegin(), buffer.cend(), [] (const uliteral_t& i) { return i == 0; })) }
			, m_fwd { new len_t*[m_empty_entries+1] }
		{
        std::fill(m_fwd,m_fwd+m_empty_entries,nullptr);
		}

		len_t rank(len_t i) const {
			DCHECK(m_bv[i]);
			return m_rank.rank(i+1);
		}

		void decode(const std::vector<len_t>& m_target_pos, const std::vector<len_t>& m_source_pos, const std::vector<len_t>& m_length) {
			const len_t factors = m_source_pos.size();
			m_env.log_stat("factors", factors);
			m_env.begin_stat_phase("Decoding Factors");
			for(len_t j = 0; j < factors; ++j) {
				const len_t& target_position = m_target_pos[j];
				const len_t& source_position = m_source_pos[j];
				const len_t& factor_length = m_length[j];
				for(len_t i = 0; i < factor_length; ++i) {
					if(m_buffer[source_position+i]) {
						decode_literal_at(target_position+i, m_buffer[source_position+i]);
					} else {
						DCHECK_EQ(m_bv[source_position+i],1);
						len_t*& bucket = m_fwd[rank(source_position+i)];
						if(bucket == nullptr) {
							bucket = new len_t[2];
							bucket[0] = 2;
							bucket[1] = target_position+i;
						}
						else
						{ // this block implements the call of m_fwd[src]->push_back(m_cursor);
							++bucket[0]; // increase the size of a bucket only by one!
							bucket = (len_t*) realloc(bucket, sizeof(len_t)*bucket[0]);
							bucket[bucket[0]-1] = target_position+i;
						}
					}

				}
				if(tdc_stats((j+1) % ((factors+5)/5) == 0 )) {
					m_env.end_stat_phase();
					m_env.begin_stat_phase("Decoding Factors at position " + std::to_string(target_position));
				}
			}
			m_env.end_stat_phase();
		}
    inline void decode_literal_at(len_t pos, uliteral_t c) {
		tdc_stats(++m_current_chain);
        tdc_stats(m_longest_chain = std::max(m_longest_chain, m_current_chain));

		DCHECK(m_buffer[pos] == 0 || m_buffer[pos] == c) << "would write " << c << " to mbuffer[" << pos << "] = " << m_buffer[pos];
        m_buffer[pos] = c;
		DCHECK(c != 0 || pos == m_buffer.size()-1); // we assume that the text to restore does not contain a NULL-byte but at its very end

		if(m_bv[pos] == 1) {
			const len_t rankpos = rank(pos);
			DCHECK_LE(rankpos, m_empty_entries);
			if(m_fwd[rankpos] != nullptr) {
				const len_t*const& bucket = m_fwd[rankpos];
				for(size_t i = 1; i < bucket[0]; ++i) {
					decode_literal_at(bucket[i], c); // recursion
				}
				delete [] m_fwd[rankpos];
				m_fwd[rankpos] = nullptr;
			}
		}

        tdc_stats(--m_current_chain);
    }

    inline len_t longest_chain() const {
        return m_longest_chain;
    }

	~LazyDecoder() {
		DCHECK(m_fwd != nullptr);
		for(size_t i = 0; i < m_empty_entries; ++i) {
			if(m_fwd[i] == nullptr) continue;
			delete [] m_fwd[i];
		}
		delete [] m_fwd;
	}


	};

class LazySuccinctListBuffer : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("esadec", "LazySuccinctListBuffer");
        m.option("lazy").dynamic("0");
        return m;

    }
    inline void decode_lazy() {
        size_t lazy = m_lazy;
        while(lazy > 0) {
            decode_lazy_();
            --lazy;
        }
    }
    inline void decode_eagerly() {
		{
			env().begin_stat_phase("Initialize Bit Vector");
			LazyDecoder decoder(this->env(),m_buffer);
			env().end_stat_phase();
			decoder.decode(m_target_pos, m_source_pos, m_length);
			tdc_stats(m_longest_chain = decoder.longest_chain());
			env().begin_stat_phase("Destructor LazyDecoder");
		}
		env().end_stat_phase();
    }

private:
    inline void decode_lazy_() {
        const len_t factors = m_source_pos.size();
        for(len_t j = 0; j < factors; ++j) {
            const len_t& target_position = m_target_pos[j];
            const len_t& source_position = m_source_pos[j];
            const len_t& factor_length = m_length[j];
            for(len_t i = 0; i < factor_length; ++i) {
				m_buffer[target_position+i] = m_buffer[source_position+i];
            }
        }
    }
    const size_t m_lazy; // number of lazy rounds

    len_t m_cursor;

	IntVector<uliteral_t> m_buffer;

    //storing factors
    std::vector<len_t> m_target_pos;
    std::vector<len_t> m_source_pos;
    std::vector<len_t> m_length;


	tdc_stats(len_t m_longest_chain = 0);

public:
    LazySuccinctListBuffer(LazySuccinctListBuffer&& other)
        : Algorithm(std::move(*this))
		, m_lazy(std::move(other.m_lazy))
        , m_cursor(std::move(other.m_cursor))
        , m_buffer(std::move(other.m_buffer))
    { }

    inline LazySuccinctListBuffer(Env&& env, len_t size)
        : Algorithm(std::move(env))
		, m_lazy(this->env().option("lazy").as_integer())
		, m_cursor(0)
		, m_buffer(size,0) 
	{ }

    inline void decode_literal(uliteral_t c) {
        m_buffer[m_cursor++] = c;
		DCHECK(c != 0 || m_cursor == m_buffer.size()); // we assume that the text to restore does not contain a NULL-byte but at its very end
    }

    inline void decode_factor(const len_t source_position, const len_t factor_length) {
        bool factor_stored = false;
        for(len_t i = 0; i < factor_length; ++i) {
            const len_t src_pos = source_position+i;
            if(m_buffer[src_pos]) {
                m_buffer[m_cursor] = m_buffer[src_pos];
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

    inline len_t longest_chain() const {
        return m_longest_chain;
    }

    inline void write_to(std::ostream& out) const {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

