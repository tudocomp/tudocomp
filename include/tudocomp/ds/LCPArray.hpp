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
    size_t m_max;

public:
    inline LCPArray() : m_max(0) {
    }

    inline iv_t& data() {
        return m_lcp;
    }

    inline const iv_t& data() const {
        return m_lcp;
    }

    inline size_t max_lcp() const {
        return m_max;
    }

    inline void construct(ITextDSProvider& t, bool consume_phi = false) {
        auto sa = t.require_sa();
        auto n = sa.size();

        //Require Phi, then consume it and either work in-place, or
        //create a new vector for the pre-LCP array.
        t.require_phi();

        std::unique_ptr<PhiArray> phi_consumed;
        const PhiArray* phi;

        iv_t *plcp_ptr;

        if(consume_phi) {
            //consume the Phi array to construct the pre-LCP array in-place
            DLOG(INFO) << "construct plcp in-place";

            phi_consumed = t.release_phi();
            phi = &*phi_consumed;
            
            plcp_ptr = &phi_consumed->m_phi;
        } else {
            //allocate a new int vector for the pre-LCP array
            phi = &t.require_phi();
            plcp_ptr = new iv_t(n, 0, bitsFor(n));
        }
        iv_t& plcp = *plcp_ptr;

        //Construct LCP using PHI
        m_max = 0;
        for(size_t i = 0, l = 0; i < n - 1; i++) {
            size_t phii = (*phi)[i];
            while(t[i+l] == t[phii+l]) {
                l++;
            }

            plcp[i] = l;
            if(l) {
                m_max = std::max(m_max, l);
                --l;
            }
        }

        //bit compress
        m_lcp = iv_t(n, 0, bitsFor(m_max));
        for(size_t i = 1; i < n; i++) {
            m_lcp[i] = plcp[sa[i]];
        }

        //post steps
        if(!phi_consumed) {
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

