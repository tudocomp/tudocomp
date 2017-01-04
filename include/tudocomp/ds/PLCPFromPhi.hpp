#pragma once

#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the PLCP array using the phi array.
class PLCPFromPhi : public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("plcp", "from_phi");
        return m;
    }

    using ArrayDS::ArrayDS;

    template<typename textds_t>
    inline void construct(textds_t& t) {
        const size_t n = t.size();

        // Construct Phi and attempt to work in-place
        m_data = t.inplace_phi();

        // Use Phi algorithm to compute PLCP array
        env().begin_stat_phase("Construct PLCP Array");

        for(len_t i = 0, l = 0; i < n - 1; ++i) {
			const len_t phii = (*m_data)[i];
			while(t[i+l] == t[phii+l]) ++l;
			(*m_data)[i] = l;
			if(l) --l;
		}

        env().end_stat_phase();
    }

};

} //ns
