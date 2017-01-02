#pragma once

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/def.hpp>
#include "forward.hpp"

#include <tudocomp/util/divsufsort.hpp>

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
	DCHECK_EQ(t[len-1],0);

	//Use divsufsort to construct
	saidx_t *sa = new saidx_t[len];
	divsufsort(t.text(), sa, len);

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

