#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the PLCP array using the phi array.
class PLCPFromPhi : public ArrayDS {
private:
    len_t m_max;

public:
    inline static Meta meta() {
        Meta m("plcp", "from_phi");
        return m;
    }

    using ArrayDS::ArrayDS;

    template<typename textds_t>
    inline void construct(textds_t& t, CompressMode cm) {
        const size_t n = t.size();

        // Construct Phi and attempt to work in-place
        m_data = t.inplace_phi(cm);

        // Use Phi algorithm to compute PLCP array
        env().begin_stat_phase("Construct PLCP Array");

        m_max = 0;
        for(len_t i = 0, l = 0; i < n - 1; ++i) {
			const len_t phii = (*m_data)[i];
			while(t[i+l] == t[phii+l]) ++l;
            m_max = std::max(m_max, l);
			(*m_data)[i] = l;
			if(l) --l;
		}

        if(cm == CompressMode::direct || cm == CompressMode::delayed) {
            compress();
        }

        env().log_stat("bit_width", size_t(m_data->width()));
        env().log_stat("size", m_data->bit_size() / 8);
        env().end_stat_phase();
    }

	inline len_t max_lcp() const {
		return m_max;
	}

    void compress() {
        DCHECK(m_data);
        m_data->width(bits_for(m_max));
        m_data->shrink_to_fit();
    }
};

} //ns
