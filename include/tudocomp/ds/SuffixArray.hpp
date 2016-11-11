#ifndef _INCLUDED_DS_SUFFIX_ARRAY_HPP
#define _INCLUDED_DS_SUFFIX_ARRAY_HPP


#include <divsufsort.h>
#include <divsufsort64.h>

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/util.hpp>
#include "forward.hpp"
namespace tdc {

template<class T>
class SuffixArray {

public:
    typedef sdsl::int_vector<> iv_t;

private:
    iv_t m_sa;


public:
    inline iv_t& data() {
        return m_sa;
    }

    inline const iv_t& data() const {
        return m_sa;
    }

    inline iv_t::value_type operator[](iv_t::size_type i) const {
        return m_sa[i];
    }

    inline iv_t::size_type size() const {
        return m_sa.size();
    }

    inline void construct(T& t);
};

}//ns
namespace tdc {

template<class T>
void SuffixArray<T>::construct(T& t) {
	const size_t len = t.size();

	//TODO:  t.text(); should do the job?
	uint8_t* copy = new uint8_t[len + 1];
	for(size_t i = 0; i < len; i++) {
		copy[i] = t[i];
	}
	copy[len] = 0;

	//TODO: with int32_t we can only create SA for texts less than 4GB
	// should be divsufsort64

	//Use divsufsort to construct
	int32_t *sa = new int32_t[len + 1];
	divsufsort(copy, sa, len + 1);

	delete[] copy;

	//Bit compress using SDSL
	size_t w = bits_for(len + 1);
	m_sa = iv_t(len + 1, 0, w);

	for(size_t i = 0; i < len + 1; i++) {
		m_sa[i]  = sa[i];
        DCHECK_LT(m_sa[i], len+1);
	}

	delete[] sa;
}

/** 
 * Computes the value BWT[i] of a text T given its suffix array SA
 * Runs in O(1) time since BWT[i] = SA[(T[i]-1) mod |SA|]
 */ 
template<typename text_t, typename sa_t>
inline typename text_t::value_type bwt(const text_t& text, const sa_t& sa, const size_t i) {
	return (sa[i] == 0) ? text[sa.size()-1] : text[sa[i]-1];
}

}//ns

#endif

