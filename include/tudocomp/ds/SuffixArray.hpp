#pragma once

#include <tudocomp/io.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/def.hpp>
#include "forward.hpp"

#include <tudocomp/util/divsufsort.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

template<class T>
class SuffixArray {

public:
    typedef DynamicIntVector iv_t;

private:
    iv_t m_sa;


public:
    inline iv_t& data() {
        return m_sa;
    }

    inline const iv_t& data() const {
        return m_sa;
    }

    inline len_t operator[](size_t i) const {
        return m_sa[i];
    }

    inline size_t size() const {
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

    //Allocate
    const size_t w = bits_for(len);
	m_sa = iv_t(len, 0, w + 1); //divsufsort needs one additional bit for signs

	//Use divsufsort to construct
	divsufsort(t.text(), m_sa, len);

    //Shrink to required width
    m_sa.width(w);
}

}//ns

