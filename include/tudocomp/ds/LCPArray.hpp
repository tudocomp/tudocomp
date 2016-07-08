#ifndef _INCLUDED_DS_LCP_ARRAY_HPP
#define _INCLUDED_DS_LCP_ARRAY_HPP

#include <tudocomp/ds/SuffixArray.hpp>

namespace tudocomp {

class LCPArray {

public:
    typedef sdsl::int_vector<> iv_t;

private:
    sdsl::int_vector<> m_lcp;

public:
    inline LCPArray(io::InputView& in, const SuffixArray& sa) {
        m_lcp = sdsl::int_vector<>(sa.size(), 0, bitsFor(sa.size()));

        //copy SA
        for(size_t i = 0; i < sa.size(); i++) {
            m_lcp[i] = sa[i];
        }

        auto text = (const uint8_t*)in.data();
        auto isa = sa.isa;
        auto n = sa.size();

        //Kasai
        for(iv_t::size_type i=0,j=0,sa_1=0,l=0; i < n; ++i) {
            sa_1 = isa[i];
            if (sa_1) {
                j = m_lcp[sa_1-1];
                if (l) --l;
                assert(i!=j);
                while (text[i+l]==text[j+l]) { // i+l < n and j+l < n are not necessary, since text[n]=0 and text[i]!=0 (i<n) and i!=j
                    ++l;
                }
                m_lcp[ sa_1-1 ] = l;
            } else {
                l = 0;
                m_lcp[ n-1 ] = 0;
            }
        }

        for (iv_t::size_type i=m_lcp.size(); i>1; --i) {
            m_lcp[i-1] = m_lcp[i-2];
        }
        m_lcp[0] = 0;
    }

    const iv_t& lcp = m_lcp;

    inline iv_t::value_type operator[](iv_t::size_type i) const {
        return m_lcp[i];
    }

    inline iv_t::size_type size() const {
        return m_lcp.size();
    }
};

}

#endif

