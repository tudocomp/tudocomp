#pragma once

#include <divsufsort.h>
#include <divsufsort64.h>

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/def.hpp>
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

namespace sa {

	inline int32_t suffix_sort(const uint8_t* T, int32_t* SA, int32_t n) {
		return divsufsort(T,SA,n);
	}
	inline int32_t suffix_sort(const uint8_t* T, int64_t* SA, int64_t n) {
		return divsufsort64(T,SA,n);
	}

}


template<class T>
void SuffixArray<T>::construct(T& t) {
	const size_t len = t.size();
	DCHECK_EQ(t[len-1],0); 


	//TODO: with int32_t we can only create SA for texts less than 4GB
	// should be divsufsort64

	//Use divsufsort to construct
	typedef std::make_signed<len_t>::type sufsort_t;
	sufsort_t *sa = new sufsort_t[len];
	sa::suffix_sort(t.text(), sa, len);

	//Bit compress using SDSL
	const size_t w = bits_for(len);
	m_sa = iv_t(len, 0, w);

	for(size_t i = 0; i < len; i++) {
		m_sa[i]  = sa[i];
        DCHECK_LT(m_sa[i], len);
	}

	delete[] sa;
}

}//ns

