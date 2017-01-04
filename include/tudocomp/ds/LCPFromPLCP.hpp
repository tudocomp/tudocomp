#pragma once

#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the LCP array using the Phi algorithm.
class LCPFromPLCP : public ArrayDS {
private:
    len_t m_max;

public:
    inline static Meta meta() {
        Meta m("lcp", "from_phi");
        return m;
    }

    using ArrayDS::ArrayDS;

    template<typename textds_t>
    inline void construct(textds_t& t) {
        // Construct Suffix Array and PLCP Array
        auto& sa = t.require_sa();
        auto& plcp = t.require_plcp();

        const size_t n = t.size();
        const size_t w = bits_for(n);

        // Compute LCP array
        env().begin_stat_phase("Construct LCP Array");

        m_data = std::make_unique<iv_t>(n, 0, w);
        m_max = 0;
        (*m_data)[0] = 0;
		for(len_t i = 1; i < n; i++) {
            const len_t x = plcp[sa[i]];
            m_max = std::max(x, m_max);
			(*m_data)[i] = x;
		}

        env().end_stat_phase();
    }

	inline len_t max_lcp() const {
		return m_max;
	}
};

} //ns
