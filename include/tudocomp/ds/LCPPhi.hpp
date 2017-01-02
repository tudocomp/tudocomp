#pragma once

#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the LCP array using the Phi algorithm.
class LCPPhi : public ArrayDS {
private:
    len_t m_max;

public:
    inline static Meta meta() {
        Meta m("lcp", "phi");
        return m;
    }

    using ArrayDS::ArrayDS;

    template<typename textds_t>
    inline void construct(textds_t& t) {
        // Construct Suffix Array
        auto& sa = t.require_sa();

        env().begin_stat_phase("Construct LCP Array");

        const size_t n = t.size();
        const size_t w = bits_for(n);

        // Construct Phi Array
        env().begin_stat_phase("Construct Phi Array");
        iv_t p(n, 0, w);
        for(size_t i = 1, prev = sa[0]; i < n; i++) {
            p[sa[i]] = prev;
            prev = sa[i];
        }
        p[sa[0]] = sa[n-1];
        env().end_stat_phase();

        // Use Phi algorithm to compute PLCP array
        env().begin_stat_phase("Phi Algorithm");
        for(len_t i = 0, l = 0; i < n - 1; ++i) {
			const len_t phii = p[i];
			while(t[i+l] == t[phii+l]) ++l;
			p[i] = l;
			if(l) --l;
		}
        env().end_stat_phase();

        // Compute LCP array
        env().begin_stat_phase("Compute LCP");
        m_data = iv_t(n, 0, w);
        m_max = 0;
        m_data[0] = 0;
		for(len_t i = 1; i < n; i++) {
            const len_t x = p[sa[i]];
            m_max = std::max(x, m_max);
			m_data[i] = x;
		}
        env().end_stat_phase();

        env().end_stat_phase();
    }

	inline len_t max_lcp() const {
		return m_max;
	}
};

} //ns
