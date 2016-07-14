#ifndef _INCLUDED_DS_PHI_ARRAY_HPP
#define _INCLUDED_DS_PHI_ARRAY_HPP

#include <tudocomp/util.h>
#include <tudocomp/ds/ITextDSProvider.hpp>
#include <tudocomp/ds/SuffixArray.hpp>

namespace tudocomp {

using io::InputView;

class PhiArray {

public:
    typedef sdsl::int_vector<> iv_t;

private:
    iv_t m_phi;

public:
    inline PhiArray() {
    }

    const iv_t& phi = m_phi;

    inline iv_t::value_type operator[](iv_t::size_type i) const {
        return m_phi[i];
    }

    inline iv_t::size_type size() const {
        return m_phi.size();
    }

    inline void construct(ITextDSProvider& t) {
        auto sa = t.require_sa();
        auto n = sa.size();

        m_phi = iv_t(n, 0, bitsFor(n));
        for(size_t i = 0, prev = 0; i < n; i++) {
            auto s = sa[i];
            m_phi[s] = prev;
            prev = s;
        }
    }
};

}

#endif

