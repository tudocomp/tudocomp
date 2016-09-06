#ifndef _INCLUDED_DS_LCP_ARRAY_HPP
#define _INCLUDED_DS_LCP_ARRAY_HPP

#include <tudocomp/util.h>
#include <sdsl/rank_support.hpp> // for the rank data structure

namespace tudocomp {
class SuffixArray;
class PhiArray;
class TextDS;

using io::InputView;

constexpr int bits = 0;
typedef size_t len_t;



//template<int bits>
class LCPArray {
    typedef sdsl::int_vector<bits> iv_t;

public:

private:
    iv_t m_lcp;
	len_t m_max;

    template <typename sa_t, int T = bits, typename std::enable_if<T == 0,int>::type = 0>
	inline void construct_lcp_array(const iv_t& plcp, const sa_t& sa) {
        const auto& n = sa.size();
		m_max = bitsFor(*std::max_element(plcp.begin(),plcp.end()));
		m_lcp = iv_t(n, 0, bitsFor(m_max));
		for(len_t i = 1; i < n; i++) {
			m_lcp[i] = plcp[sa[i]];
		}
	}

    template <typename sa_t, int T = bits, typename std::enable_if<T != 0,int>::type = 0>
	inline void construct_lcp_array(const iv_t& plcp, const sa_t& sa) {
		m_max = bitsFor(*std::max_element(plcp.begin(),plcp.end()));
        const auto& n = sa.size();
		for(len_t i = 1; i < n; i++) {
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

    inline void construct(TextDS& t);
};




template<
typename sa_t,
typename select_t = sdsl::select_support_mcl<1,1>>
class lcp_sada {
    typedef sdsl::int_vector<bits> iv_t;
	sa_t* sa;
	sdsl::bit_vector bv;
	select_t s;
	public:
	inline len_t operator[](len_t i) const;
    inline void construct(TextDS& t);

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
			assert(plcp[i+1]+1 >= plcp[i]);
			len += plcp[i+1]-plcp[i]+1;
		}
		sdsl::bit_vector bv(len+2*n,0);
		bv[0]=1;
		len=0;
		for(len_t i = 0; i+1 < n; i++) {
			len += plcp[i+1]-plcp[i]+2;
			assert(len < bv.size());
			bv[len] = 1;
		}
		return bv;
	}
	inline static iv_t phi_algorithm(TextDS& t) {
		auto& sa = t.require_sa();
		const auto n = sa.size();
		t.require_phi();
		std::unique_ptr<PhiArray> phi(t.release_phi());
		iv_t plcp(std::move(phi->data()));
		for(len_t i = 0, l = 0; i < n - 1; ++i) {
			const len_t phii = (*phi)[i];
			while(t[i+l] == t[phii+l]) {
				l++;
			}
			plcp[i] = l;
			if(l) {
				--l;
			}
		}
		return plcp;
	}
	template<class lcp_t, class sa_t>
	iv_t create_plcp_array(const lcp_t& lcp, const sa_t& isa) {
		const len_t n = isa.size();
		iv_t plcp(n);
		for(len_t i = 0; i < n; ++i) {
			assert(isa[i] < n);
			assert(isa[i] >= 0);
			plcp[i] = lcp[isa[i]];
		}
		return plcp;
	}
}

template< typename sa_t, typename select_t>
inline len_t lcp_sada<sa_t,select_t>::operator[](len_t i) const {
	const len_t idx = (*sa)[i];
	return s.select(idx+1) - 2*idx;
}

template< typename sa_t, typename select_t>
inline void lcp_sada<sa_t,select_t>::construct(TextDS& t) {
	iv_t plcp(LCP::phi_algorithm(t));
	bv(LCP::construct_lcp_sada(plcp));
	s.set_vector(&bv); 
}

template< typename sa_t, typename select_t>
inline len_t lcp_sada<sa_t,select_t>::size() const {
	return sa->size();
}

inline void LCPArray::construct(TextDS& t) {
	iv_t plcp(LCP::phi_algorithm(t));
	construct_lcp_array(plcp, t.require_sa());
}

}

#endif

