#pragma once
#include <tudocomp/config.h>

#include <vector>

#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/ds/ArrayMaxHeap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <sdsl/select_support_mcl.hpp> // for the select data structure

namespace tdc {
namespace lcpcomp {


template<class plcp_t>
inline static sdsl::bit_vector construct_plcp_bitvector(const plcp_t& plcp) {
	const len_t n = plcp.size();
	len_t len = plcp[0];
	for(len_t i = 0; i+1 < n; i++) {
		DCHECK_GE(plcp[i+1]+1, plcp[i]);
		len += plcp[i+1]-plcp[i]+1;
	}
	sdsl::bit_vector bv(len+n,0);
	bv[plcp[0]]=1;
	len=plcp[0];
	for(len_t i = 0; i+1 < n; i++) {
		len += plcp[i+1]-plcp[i]+2;
		DCHECK_LT(len, bv.size());
		bv[len] = 1;
	}
	DCHECK_EQ(len, bv.size()-1);
	DCHECK_EQ(plcp.size(), std::accumulate(bv.begin(), bv.end(), 0));
	return bv;
}

/**
 * Constructs the phi array with phi[sa[i]] = sa[i-1]
 * @param sa the suffix array
 * @return the phi array
 */
template<class phi_t, class sa_t>
inline phi_t construct_phi_array(const sa_t& sa) {
	const len_t n = sa.size();
	//assert_permutation(sa,n);

	phi_t phi(n, 0, bits_for(n));
	for(size_t i = 1, prev = sa[0]; i < n; i++) {
		phi[sa[i]] = prev;
		prev = sa[i];
	}
	phi[sa[0]] = sa[n-1];
	//assert_permutation(phi,n);
	return phi;
}

/**
 * Constructs the PLCP array
 * @param phi the phi-array. Will be overwritten with PLCP
 * @text the input text
 * @author Kärkkäinen et. al, "Permuted Longest-Common-Prefix Array", CPM'09
 */
template<typename phi_t, typename text_t>
inline void phi_algorithm(phi_t& phi, const text_t& text) {
	const size_t n = phi.size();
	DCHECK_EQ(text[n-1],0);
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
	phi[n-1]=0;
}



template<
typename sa_t,
typename select_t = sdsl::select_support_mcl<1,1>>
class lcp_sada {
	const sa_t& m_sa;
	const sdsl::bit_vector m_bv;
	const select_t m_select;
	public:
	lcp_sada(const sa_t& sa, const sdsl::bit_vector&& bv) 
		: m_sa(sa)
		, m_bv(bv)
		, m_select(&m_bv)
	{
	}
	len_t operator[](len_t i) const {
		if(size() == 1) return 0;
		const len_t idx = m_sa[i];
		return m_select.select(idx+1) - 2*idx;
	}
	len_t plcp(len_t idx) const {
		if(size() == 1) return 0;
		return m_select.select(idx+1) - 2*idx;
	}
    inline len_t size() const {
		return m_sa.size();
	}

	// only for reference:
	// len_t naive_select(len_t idx) const {
	// 	DCHECK_GT(m_bv.size(), 1);
	// 	DCHECK_GT(idx,0);
	// 	DCHECK_LT(idx, m_bv.size());
	// 	const len_t chunk_size = 1 + ((m_bv.size()-1)/64);
	// 	len_t pos = 0;
	// 	len_t rank = 0;
	// 	const uint64_t*const data = m_bv.data();
	// 	for(pos = 0; pos < chunk_size; ++pos) {
	// 		const uint_fast8_t ones = sdsl::bits::cnt(data[pos]);
	// 		if(rank+ones >= idx) break;
	// 		rank += ones;
	// 	}
	// 	if(pos == chunk_size) return m_bv.size();
	// 	return 64*pos + sdsl::bits::sel(data[pos], idx-rank);
	// }
    //
	// len_t naive_plcp(len_t idx) const {
	// 	if(size() == 1) return 0;
	// 	return naive_select(idx+1) - 2*idx;
	// }

};

class LCPForwardIterator {
	sdsl::bit_vector m_bv;
	
	len_t m_idx = 0; // current select parameter
	len_t m_block = 0; // block index
	len_t m_blockrank = 0; //number of ones up to previous block

	public:
	LCPForwardIterator(sdsl::bit_vector&& bv) : m_bv(bv) {}

	len_t index() const { return m_idx; }

	len_t next_select() {
		DCHECK_GT(m_bv.size(), 1);
//		DCHECK_GT(m_idx,0);
		DCHECK_LT(m_idx+1, m_bv.size());
		const len_t chunk_size = 1 + ((m_bv.size()-1)/64); //TODO: in constructor
		const uint64_t*const data = m_bv.data();
		while(m_block < chunk_size) {
			const uint_fast8_t ones = sdsl::bits::cnt(data[m_block]); // TODO: make member variable to speed up
			if(m_blockrank+ones >= m_idx+1) break;
			m_blockrank += ones;
			++m_block;
		}
		if(m_block == chunk_size) return m_bv.size();
		return 64*m_block + sdsl::bits::sel(data[m_block], m_idx+1-m_blockrank);

	}
	len_t operator()() {
		const len_t ret = next_select() - 2*m_idx;
		return ret;
	}
	void advance() {
		++m_idx;
	}
};


template<class sa_t, class text_t, class select_t = sdsl::select_support_mcl<1,1>>
sdsl::bit_vector construct_plcp_bitvector(Env& env, const sa_t& sa, const text_t& text) {
	typedef DynamicIntVector phi_t;
	env.begin_stat_phase("Construct Phi Array");
	phi_t phi { construct_phi_array<phi_t,sa_t>(sa) };
	env.end_stat_phase();
	env.begin_stat_phase("Phi-Algorithm");
	phi_algorithm(phi, text); 
	env.end_stat_phase();
	env.begin_stat_phase("Build Sada Bit Vector");
	auto ret = construct_plcp_bitvector(phi);
	env.log_stat("bit vector length", ret.bit_size());
	env.end_stat_phase();
	return ret;
}
	
template<class sa_t, class text_t, class select_t = sdsl::select_support_mcl<1,1>>
lcp_sada<sa_t,select_t> construct_lcp_sada(Env& env, const sa_t& sa, const text_t& text) {
	sdsl::bit_vector bv = construct_plcp_bitvector(env, sa, text);
	env.begin_stat_phase("Build Select on Bit Vector");
	auto ret = lcp_sada<sa_t,select_t> { sa, std::move(bv) };
	env.end_stat_phase();
	return ret;
}




/// A very naive selection strategy for LCPComp.
///
/// TODO: Describe
class PLCPStrategy : public Algorithm {
private:
    typedef TextDS<> text_t;

public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("lcpcomp_comp", "plcp");
        return m;
    }

    inline void factorize(text_t& text,
                   size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
		env().begin_stat_phase("Construct index ds");
		text.require(text_t::SA | text_t::ISA);

        const auto& sa = text.require_sa();
        const auto& isa = text.require_isa();

		// TESTS
        // auto lcpp = text.release_plcp();
        // auto lcp_datap = lcpp->relinquish();
        // auto& plcp = *lcp_datap;
        //
		// {
		// 	typedef std::remove_reference<decltype(sa)>::type sa_t;
		// 	typedef DynamicIntVector phi_t;
		// 	phi_t phi { construct_phi_array<phi_t,sa_t>(sa) };
		// 	phi_algorithm(phi, text); 
		// 	for(size_t i = 0; i < plcp.size()-1; ++i) {
		// 		DCHECK_EQ(plcp[i], phi[i]);
		// 	}
		// }
        //
        //
		// auto lcp = construct_lcp_sada(env(), sa, text);
		// for(size_t i = 0; i < plcp.size()-1; ++i) {
		// 	DCHECK_EQ(plcp[i], lcp.plcp(i));
		// }
		// // for(size_t i = 0; i < plcp.size()-1; ++i) {
		// // 	DCHECK_EQ(plcp[i], lcp.naive_plcp(i));
		// // }
		// LCPForwardIterator pplcp { (construct_plcp_bitvector(env(), sa, text)) };
		// // for(size_t i = 0; i < plcp.size()-1; ++i) {
		// // 	DCHECK_EQ(plcp[i], pplcp());
		// // 	pplcp.advance();
		// // }

		LCPForwardIterator pplcp { (construct_plcp_bitvector(env(), sa, text)) };
        const len_t n = sa.size();

		env().end_stat_phase();
		env().begin_stat_phase("Search Peaks");

		struct Poi {
			len_t pos;
			len_t lcp;
			len_t no;
			Poi(len_t _pos, len_t _lcp, len_t _no) : pos(_pos), lcp(_lcp), no(_no) {}
			bool operator<(const Poi& o) const {
				DCHECK_NE(o.pos, this->pos);
				if(o.lcp == this->lcp) return this->pos > o.pos;
				return this->lcp < o.lcp;
			}
		};

		boost::heap::pairing_heap<Poi> heap;
		std::vector<boost::heap::pairing_heap<Poi>::handle_type> handles;

		IF_STATS(len_t max_heap_size = 0);

		// std::stack<poi> pois; // text positions of interest, i.e., starting positions of factors we want to replace

		len_t lastpos = 0;
		len_t lastpos_lcp = 0;
		for(len_t i = 0; i+1 < n; ++i) {
			while(pplcp.index() < i) pplcp.advance();
			const len_t plcp_i = pplcp(); DCHECK_EQ(pplcp.index(), i);
			if(heap.empty()) {
				if(plcp_i >= threshold) {
					handles.emplace_back(heap.emplace(i, plcp_i, handles.size()));
					lastpos = i;
					lastpos_lcp = plcp_i;
				}
				continue;
			}
			if(i - lastpos >= lastpos_lcp || tdc_unlikely(i+1 == n)) {
				IF_DEBUG(bool first = true);
				IF_STATS(max_heap_size = std::max<len_t>(max_heap_size, heap.size()));
				DCHECK_EQ(heap.size(), handles.size());
				while(!heap.empty()) {
					const Poi& top = heap.top();
					const len_t source_position = sa[isa[top.pos]-1];
					factors.emplace_back(top.pos, source_position, top.lcp);
					const len_t next_pos = top.pos; // store top, this is the current position that gets factorized
					IF_DEBUG(if(first) DCHECK_EQ(top.pos, lastpos); first = false;)

					{
						len_t newlcp_peak = 0; // a new peak can emerge at top.pos+top.lcp
						bool peak_exists = false;
						if(top.pos+top.lcp < i) 
						for(len_t j = top.no+1; j < handles.size(); ++j) { // erase all right peaks that got substituted
							if( handles[j].node_ == nullptr) continue;
							const Poi poi = *(handles[j]);
							DCHECK_LT(next_pos, poi.pos);
							if(poi.pos < next_pos+top.lcp) { 
								heap.erase(handles[j]);
								handles[j].node_ = nullptr;
								if(poi.lcp + poi.pos > next_pos+top.lcp) {
									const len_t remaining_lcp = poi.lcp+poi.pos - (next_pos+top.lcp);
									DCHECK_NE(remaining_lcp,0);
									if(newlcp_peak != 0) DCHECK_LE(remaining_lcp, newlcp_peak); 
									newlcp_peak = std::max(remaining_lcp, newlcp_peak);
								}
							} else if( poi.pos == next_pos+top.lcp) { peak_exists=true; }
							else { break; }  // only for performance
						}
#ifdef DEBUG
						if(peak_exists) {  //TODO: DEBUG
							for(len_t j = top.no+1; j < handles.size(); ++j) { 
								if( handles[j].node_ == nullptr) continue;
								const Poi& poi = *(handles[j]);
								if(poi.pos == next_pos+top.lcp) {
									DCHECK_LE(newlcp_peak, poi.lcp);
									break;
								}
							}
						}
#endif
						if(!peak_exists && newlcp_peak >= threshold) {
							len_t j = top.no+1;
							DCHECK(handles[j].node_ == nullptr);
							handles[j] = heap.emplace(next_pos+top.lcp, newlcp_peak, j);
						}
						
					}
					handles[top.no].node_ = nullptr;
					heap.pop(); // top now gets erased

					for(auto it = handles.rbegin(); it != handles.rend(); ++it) {
						if( (*it).node_ == nullptr) continue;
						Poi& poi = (*(*it));
						if(poi.pos > next_pos)  continue;
						const len_t newlcp = next_pos - poi.pos;
						if(newlcp < poi.lcp) {
							if(newlcp < threshold) {
								heap.erase(*it);
								it->node_ = nullptr;
							} else {
								poi.lcp = newlcp;
								heap.decrease(*it);

							}
						} else {
							break;
						}
					}
				}
				handles.clear();
				--i;
				continue;
			}
			DCHECK_EQ(pplcp.index(), i);
			DCHECK_EQ(plcp_i, pplcp());
			if(plcp_i <= lastpos_lcp) continue;
			DCHECK_LE(threshold, plcp_i);
			handles.emplace_back(heap.emplace(i,plcp_i, handles.size()));
			lastpos = i;
//			DCHECK_EQ(plcp[lastpos], plcp_i);
			lastpos_lcp = plcp_i;
		}
        IF_STATS(env().log_stat("max heap size", max_heap_size));
        env().end_stat_phase();
    }

};

}}//ns

