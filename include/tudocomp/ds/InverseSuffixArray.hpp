#ifndef _INCLUDED_DS_INVERSE_SUFFIX_ARRAY_HPP
#define _INCLUDED_DS_INVERSE_SUFFIX_ARRAY_HPP

#include <tudocomp/util.h>
#include "forward.hh"

namespace tdc {


template<class T>
class InverseSuffixArray {

public:
    typedef sdsl::int_vector<> iv_t;

private:
    iv_t m_isa;

public:
    inline InverseSuffixArray() {
    }

    inline iv_t& data() {
        return m_isa;
    }

    inline const iv_t& data() const {
        return m_isa;
    }

    inline iv_t::value_type operator[](iv_t::size_type i) const {
        return m_isa[i];
    }

    inline iv_t::size_type size() const {
        return m_isa.size();
    }
    inline void construct(T& t);

};

}//ns
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/SuffixArray.hpp>
namespace tdc {

template<class T>
inline void InverseSuffixArray<T>::construct(T& t) {
	auto& sa = t.require_sa();
	auto n = sa.size();

	m_isa = iv_t(n, 0, bits_for(n));
	for(size_t i = 0; i < n; i++) {
        DCHECK_LT(sa[i], n);
		m_isa[sa[i]] = i;
	}
}

}

#endif

