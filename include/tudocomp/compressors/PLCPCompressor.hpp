#pragma once

#include <tudocomp/util.hpp>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/compressors/ESACompressor.hpp>


namespace tdc {
namespace plcp {


template<class lcp_t>
class MaxPLCPHeap {

private:
    enum perlocation_dir_t {
        NONE,
        LEFT,
        RIGHT
    };

    static inline len_t lc(len_t i) {
        return 2*i+1;
    }

    static inline len_t rc(len_t i) {
        return 2*i+2;
    }

    static inline len_t parent(len_t i) {
        return (i-1)/2;
    }

    // data backend
    const lcp_t* m_lcp;

    // undefined position in heap
    size_t m_undef;

    // heap
    size_t m_size;
    DynamicIntVector m_heap;

    // back mapping
    DynamicIntVector m_pos;

    inline void put(size_t pos, len_t i) {
        m_heap[pos] = i;
        m_pos[i] = pos;
    }

public:
    /// Constructor
    inline MaxPLCPHeap(const lcp_t& plcp, size_t threshold)
        : m_lcp(&plcp), m_size(0)
	{
        auto n = plcp.size();

        size_t num_entries = 0;
        for(size_t i = 1; i < n; i++) {
			if(plcp[i-1] < plcp[i] && plcp[i] >= threshold) {
				++num_entries;
			}
        }

        m_heap = DynamicIntVector(num_entries, 0, bits_for(n-1));
        m_undef = num_entries;
        m_pos = DynamicIntVector(n, m_undef, bits_for(m_undef));

        //Construct heap
        for(size_t i = 1; i < n; i++) {
			if(plcp[i-1] < plcp[i] && plcp[i] >= threshold) {
				insert(i);
			}
        }
    }

    /// Insert suffix array item with index i.
    inline void insert(len_t i) {
        size_t pos = m_size++;

        // perlocate up
        auto lcp_i = (*m_lcp)[i];
        while(pos > 0 && lcp_i > (*m_lcp)[m_heap[parent(pos)]]) {
            put(pos, m_heap[parent(pos)]);
            pos = parent(pos);
        }

        put(pos, i);
    }

private:
    inline void perlocate_down(size_t pos, len_t k) {
        auto lcp_k = (*m_lcp)[k];

        perlocation_dir_t dir = NONE;
        do {
            len_t lcp_lc = (lc(pos) < m_size) ? (*m_lcp)[m_heap[lc(pos)]] : 0;
            len_t lcp_rc = (rc(pos) < m_size) ? (*m_lcp)[m_heap[rc(pos)]] : 0;

            // find perlocation direction
            if(lcp_k < lcp_lc && lcp_k < lcp_rc) {
                // both children are larger, pick the largest
                dir = (lcp_lc > lcp_rc) ? LEFT : RIGHT;
            } else if(lcp_k < lcp_lc) {
                // go to the left
                dir = LEFT;
            } else if(lcp_k < lcp_rc) {
                // go to the right
                dir = RIGHT;
            } else {
                dir = NONE;
            }

            // go down if necessary
            if(dir == LEFT) {
                // left
                put(pos, m_heap[lc(pos)]);
                pos = lc(pos);
            } else if(dir == RIGHT) {
                // right
                put(pos, m_heap[rc(pos)]);
                pos = rc(pos);
            }
        } while(dir != NONE);

        put(pos, k);
    }

public:
    /// Remove suffix array item with index i.
    inline void remove(len_t i) {
        auto pos = m_pos[i];
        if(pos != m_undef) {
            // get last element in heap
            auto k = m_heap[--m_size];

            // perlocate it down, starting at the former position of i
            perlocate_down(pos, k);
        }

        m_pos[i] = m_undef; // i was removed
    }

    /// Decrease key on array item with index i.
    inline void decrease_key(len_t i) {
        auto pos = m_pos[i];
        if(pos != m_undef) {
            // perlocate item down, starting at its current position
            perlocate_down(pos, i);
        }
    }

    /// Checks whether or not suffix array entry i is contained in this heap.
    inline bool contains(len_t i) const {
        return m_pos[i] != m_undef;
    }

    /// Get number of contained entries.
    inline size_t size() const {
        return m_size;
    }
    inline bool empty() const {
        return m_size==0;
    }

    /// Get first item (suffix array index with highest LCP)
    inline size_t top() const {
        return m_heap[0];
    }

    // for tests?
    inline bool is_valid() const {
        for(size_t i = 0; i < m_size; i++) {
            auto lcp_i = (*m_lcp)[m_heap[i]];
            if(lc(i) < m_size) DCHECK(lcp_i >= (*m_lcp)[m_heap[lc(i)]]);
            if(rc(i) < m_size) DCHECK(lcp_i >= (*m_lcp)[m_heap[rc(i)]]);
        }

        return true;
    }
};



	template<class phi_t, class sa_t>
		inline phi_t construct_phi_array(const sa_t& sa) {
			const len_t n = sa.size();
			assert_permutation(sa,n);

			phi_t phi { n, 0, bits_for(n) };
			for(size_t i = 1, prev = sa[0]; i < n; i++) {
				phi[sa[i]] = prev;
				prev = sa[i];
			}
			phi[sa[0]] = sa[n-1];
			assert_permutation(phi,n);
			return phi;
		}
	/*
	 * Constructs PLCP inplace in phi
	 */
	template<typename text_t, typename phi_t>
		inline void phi_algorithm(phi_t& phi, const text_t& text) {
			const len_t n = phi.size();
//			phi_t plcp(std::move(phi.data()));
			for(len_t i = 0, l = 0; i < n - 1; ++i) {
				const len_t phii = phi[i];
				DCHECK_LT(i+l, n);
				DCHECK_LT(phii+l, n);
				DCHECK_NE(i, phii);
				while(text[i+l] == text[phii+l]) { 
					l++;
					DCHECK_LT(i+l, n);
					DCHECK_LT(phii+l, n);
				}
				phi[i] = l;
				if(l) {
					--l;
				}
			}
		}
}//ns

template<typename coder_t,  typename dec_t, typename text_t = TextDS<>>
class PLCPCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "plcpcomp");
        m.option("coder").templated<coder_t>();
        m.option("esadec").templated<dec_t, esacomp::SuccinctListBuffer>();
        m.option("textds").templated<text_t, TextDS<>>();
        m.option("threshold").dynamic("3");
        m.needs_sentinel_terminator();
        return m;
    }

    /// Construct the class with an environment.
    inline PLCPCompressor(Env&& env) : Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();
        DCHECK(in.ends_with(uint8_t(0)));
		TextDS<> text { env().env_for_option("textds"), in };

		const len_t threshold { static_cast<len_t>(env().option("threshold").as_integer()) }; //factor threshold
		env().log_stat("threshold", threshold);


		env().begin_stat_phase("Construct text ds");
		text.require(text_t::SA | text_t::ISA | text_t::PLCP);
		env().end_stat_phase();

		const auto& sa = text.require_sa();
		auto plcpp = text.release_plcp();
		auto plcppp = plcpp->relinquish();
		auto& plcp = *plcppp;
		plcp[text.size()-1] = 0;

		// env().begin_stat_phase("Construct PLCP");
		// // const auto& sa = text.require_sa();
		// // IntVector<len_t> plcp = plcp::construct_phi_array<IntVector<len_t>>(sa);
		// // plcp::phi_algorithm(plcp,text);
		// env().end_stat_phase();

		// env().begin_stat_phase("Find POIs");
		// IntVector<len_t> pois; // text positions of interest
		// for(len_t i = 1; i+1 < plcp.size(); ++i) { 
		// 	if(plcp[i-1] < plcp[i] && plcp[i] > threshold) {
		// 		pois.push_back(i);
		// 	}
		// }
//		auto compare =  [&plcp] (const len_t& a, const len_t& b) { return plcp[a] > plcp[b]; };
		//std::sort(pois.begin(), pois.end(), [&plcp] (const len_t& a, const len_t& b) { return plcp[a] > plcp[b]; });
		plcp::MaxPLCPHeap<typename text_t::plcp_type::data_type> heap { plcp, threshold };
		env().end_stat_phase();

		env().begin_stat_phase("Factorize");
		lzss::FactorBuffer factors;
		const auto& isa = text.require_isa();

        while(heap.size() > 0) {
            const len_t target_position = heap.top();
            const len_t factor_length = plcp[target_position]; DCHECK_GE(factor_length, threshold);
            const len_t source_position = sa[isa[target_position]-1]; DCHECK_LT(target_position+1, text.size());

            factors.push_back(lzss::Factor { target_position, source_position, factor_length });

            //remove overlapped entries
            for(len_t k = 0; k < factor_length; ++k) {
				if(heap.contains(target_position+k)) {
					heap.remove(target_position+k);
				}
				plcp[target_position+k] = 0;
            }

            //correct intersecting entries
			for(len_t k = 0; k < factor_length && k < target_position; ++k) {
				const len_t affected_position = target_position - k - 1;
				if(target_position < affected_position + plcp[affected_position]) {
					const len_t new_length = target_position - affected_position;
					plcp[affected_position] = new_length;
					if(heap.contains(affected_position)) {
						if(new_length >= threshold) {
							heap.decrease_key(affected_position);
						} else {
							heap.remove(affected_position);
						}
					}
				}

			}
			const len_t next_target_position = target_position+factor_length;
			if(next_target_position+1 < text.size() && plcp[next_target_position] >= threshold && !heap.contains(next_target_position)) {
				heap.insert(next_target_position);
			}

		}
		env().end_stat_phase();
		env().log_stat("factors", factors.size());

		env().begin_stat_phase("Sorting Factors");
		// sort factors
		factors.sort();
		env().end_stat_phase();

		env().begin_stat_phase("Encode Factors");

		// encode
		typename coder_t::Encoder coder(env().env_for_option("coder"), output, lzss::TextLiterals<text_t>(text, factors));

		lzss::encode_text(coder, text, factors); //TODO is this correct?
		env().end_stat_phase();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        //TODO: tell that forward-factors are allowed
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        auto outs = output.as_stream();


        //lzss::decode_text_internal<coder_t, dec_t>(decoder, outs);
        // if(lazy == 0)
        // 	lzss::decode_text_internal<coder_t, dec_t>(decoder, outs);
        // else
        esacomp::decode_text_internal<typename coder_t::Decoder, dec_t>(env().env_for_option("esadec"), decoder, outs);
    }
};

}

