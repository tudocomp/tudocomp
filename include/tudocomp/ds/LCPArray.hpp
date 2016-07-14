#ifndef _INCLUDED_DS_LCP_ARRAY_HPP
#define _INCLUDED_DS_LCP_ARRAY_HPP

#include <tudocomp/util.h>
#include <tudocomp/ds/ITextDSProvider.hpp>
#include <tudocomp/ds/SuffixArray.hpp>
#include <tudocomp/ds/PhiArray.hpp>

namespace tudocomp {

using io::InputView;

class LCPArray {

public:
    typedef sdsl::int_vector<> iv_t;

private:
    iv_t m_lcp;

public:
    inline LCPArray() {
    }

    const iv_t& lcp = m_lcp;

    inline void construct(ITextDSProvider& t, bool consume_phi = false) {
        auto sa = t.require_sa();
        auto phi = t.require_phi();
        auto n = sa.size();

        iv_t *plcp_ptr;
        //iv_t plcp(n, 0, bitsFor(n));
        if(consume_phi) {
            //use the Phi array to construct the pre-LCP array in-place
            DLOG(INFO) << "construct plcp in-place";
            plcp_ptr = &phi.m_phi;
        } else {
            //allocate a new int vector for the pre-LCP array
            plcp_ptr = new iv_t(n, 0, bitsFor(n));
        }
        iv_t& plcp = *plcp_ptr;

        //Construct LCP using PHI
        size_t max_lcp = 0;
        for(size_t i = 0, l = 0; i < n - 1; i++) {
            size_t phii = phi[i];
            while(t[i+l] == t[phii+l]) {
                l++;
            }

            plcp[i] = l;
            if(l) {
                max_lcp = std::max(max_lcp, l);
                --l;
            }
        }

        //bit compress
        m_lcp = iv_t(n, 0, bitsFor(max_lcp));
        for(size_t i = 1; i < n; i++) {
            m_lcp[i] = plcp[sa[i]];
        }

        //post steps
        if(consume_phi) {
            t.release_phi(); //Phi has been consumed
        } else {
            delete plcp_ptr;
        }
    }

    inline iv_t::value_type operator[](iv_t::size_type i) const {
        return m_lcp[i];
    }

    inline iv_t::size_type size() const {
        return m_lcp.size();
    }
};

}

#endif

