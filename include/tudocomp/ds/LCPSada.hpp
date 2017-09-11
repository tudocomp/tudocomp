#pragma once
#include <tudocomp/def.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <sdsl/select_support_mcl.hpp> // for the select data structure

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {


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
 * @param text the input text
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
class LCPSada {
	const sa_t& m_sa;
	const sdsl::bit_vector m_bv;
	const select_t m_select;
	public:
	LCPSada(const sa_t& sa, const sdsl::bit_vector&& bv)
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
		DCHECK_GE(m_bv.size(), 1);
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

template<class sa_t, class text_t, class select_t = sdsl::select_support_mcl<1,1>>
sdsl::bit_vector construct_plcp_bitvector(Env&, const sa_t& sa, const text_t& text) {
	typedef DynamicIntVector phi_t;

    phi_t phi = StatPhase::wrap("Construct Phi Array", [&]{
        return construct_phi_array<phi_t,sa_t>(sa);
    });

    StatPhase::wrap("Phi-Algorithm", [&]{
        phi_algorithm(phi, text);
    });

    return StatPhase::wrap("Build Sada Bit Vector", [&]{
        auto ret = construct_plcp_bitvector(phi);
        StatPhase::log("bit vector length", ret.bit_size());
        return ret;
    });
}

template<class sa_t, class text_t, class select_t = sdsl::select_support_mcl<1,1>>
LCPSada<sa_t,select_t> construct_lcp_sada(Env& env, const sa_t& sa, const text_t& text) {
    return StatPhase::wrap("Build Select on Bit Vector", [&]{
        sdsl::bit_vector bv = construct_plcp_bitvector(env, sa, text);
        return LCPSada<sa_t,select_t> { sa, std::move(bv) };
    });
}

}//ns

