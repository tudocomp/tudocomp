#ifndef _INCLUDED_DS_LCP_ARRAY_HPP
#define _INCLUDED_DS_LCP_ARRAY_HPP

#include <tudocomp/util.h>
#include <sdsl/rank_support.hpp> // for the rank data structure
#include "forward.hh"

namespace tudocomp {

constexpr int bits = 0;
typedef size_t len_t;

template<class T>
class LCPArray {
    typedef sdsl::int_vector<bits> iv_t;

public:

private:
    iv_t m_lcp;
	len_t m_max;

    template <typename sa_t, int U = bits, typename std::enable_if<U == 0,int>::type = 0>
	inline void construct_lcp_array(const iv_t& plcp, const sa_t& sa) {
        const auto& n = sa.size();
		m_max = *std::max_element(plcp.begin(),plcp.end());
		m_lcp = iv_t(n, 0, bits_for(m_max));
		for(len_t i = 1; i < n; i++) { //TODO: start at 0, see line 149
			DCHECK_LT(sa[i], n);
			m_lcp[i] = plcp[sa[i]];
		}
		for(size_t i = 1; i < m_lcp.size(); ++i) { //TODO: start at 0, see line 149
			DCHECK_EQ(m_lcp[i], plcp[sa[i]]);
		}
	}

    template <typename sa_t, int U = bits, typename std::enable_if<U != 0,int>::type = 0>
	inline void construct_lcp_array(const iv_t& plcp, const sa_t& sa) {
        const auto& n = sa.size();
		m_max = bits_for(*std::max_element(plcp.begin(),plcp.end()));
        m_lcp = iv_t(n);
		for(len_t i = 1; i < n; i++) { //TODO: start at 0, see line 149
			m_lcp[i] = plcp[sa[i]];
		}
	}


public:
	/**
	 * Returns the maximum value of the LCP array
	 */
	inline len_t max_lcp() const {
		return m_max;
	}

    inline iv_t& data() {
        return m_lcp;
    }

    inline const iv_t& data() const {
        return m_lcp;
    }


    inline iv_t::value_type operator[](iv_t::size_type i) const {
        return m_lcp[i];
    }

    inline iv_t::size_type size() const {
        return m_lcp.size();
    }

    inline void construct(T& t);
};




template<
typename T,
typename sa_t,
typename select_t = sdsl::select_support_mcl<1,1>>
class lcp_sada {
    typedef sdsl::int_vector<bits> iv_t;
	sa_t* sa;
	sdsl::bit_vector bv;
	select_t s;
	public:
	inline len_t operator[](len_t i) const;
    inline void construct(T& t);

    inline len_t size() const;
	/**
	 * Returns the maximum value of the LCP array
	 */
	inline len_t max_lcp() const {
		len_t max = (*this)[0];
		for(len_t i = 1; i < sa->size(); ++i) {
			max = std::max(max, (*this)[i]);
		}
		return max;
	}
	
};





}//ns
#include <tudocomp/ds/SuffixArray.hpp>
#include <tudocomp/ds/PhiArray.hpp>
namespace tudocomp {




namespace LCP {
    typedef sdsl::int_vector<bits> iv_t;
	inline static sdsl::bit_vector construct_lcp_sada(const iv_t& plcp) {
		const len_t n = plcp.size();
		len_t len = 0;
		for(len_t i = 0; i+1 < n; i++) {
			DCHECK_GE(plcp[i+1]+1, plcp[i]);
			len += plcp[i+1]-plcp[i]+1;
		}
		sdsl::bit_vector bv(len+2*n,0);
		bv[0]=1;
		len=0;
		for(len_t i = 0; i+1 < n; i++) {
			len += plcp[i+1]-plcp[i]+2;
			DCHECK_LT(len, bv.size());
			bv[len] = 1;
		}
		return bv;
	}
	template<typename T>
	inline static iv_t phi_algorithm(T& t) {
        t.require_phi();
		std::unique_ptr<PhiArray<T>> phi(std::move(t.release_phi()));
        size_t n = phi->size();

		iv_t plcp(std::move(phi->data()));
		for(len_t i = 0, l = 0; i < n - 1; ++i) {
			const len_t phii = plcp[i];
            DCHECK_LT(i+l, n);
            DCHECK_LT(phii+l, n);
            DCHECK_NE(i, phii);
			while(t[i+l] == t[phii+l]) { // !!! UNDEFINED BEHAVIOUR, t[phi.size()-1] is not defined -> InputView needs to be checked
				l++;
                DCHECK_LT(i+l, n);
                DCHECK_LT(phii+l, n);
			}
			plcp[i] = l;
			if(l) {
				--l;
			}
		}
		return plcp;
	}
}

template<typename T, typename sa_t, typename select_t>
inline len_t lcp_sada<T,sa_t,select_t>::operator[](len_t i) const {
	const len_t idx = (*sa)[i];
	return s.select(idx+1) - 2*idx;
}

template<typename T, typename sa_t, typename select_t>
inline void lcp_sada<T,sa_t,select_t>::construct(T& t) {
	iv_t plcp(LCP::phi_algorithm(t));
	bv(LCP::construct_lcp_sada(plcp));
	s.set_vector(&bv); 
}

template<typename T, typename sa_t, typename select_t>
inline len_t lcp_sada<T,sa_t,select_t>::size() const {
	return sa->size();
}

template<class T>
inline void LCPArray<T>::construct(T& t) {
	iv_t plcp(LCP::phi_algorithm(t));
	construct_lcp_array(plcp, t.require_sa());
}

}

#endif

