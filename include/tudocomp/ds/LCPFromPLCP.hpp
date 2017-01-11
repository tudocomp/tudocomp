#pragma once

#include <tudocomp/ds/CompressMode.hpp>
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

    template<typename textds_t>
    inline LCPFromPLCP(Env&& env, textds_t& t, CompressMode cm)
            : ArrayDS(std::move(env)) {

        // Construct Suffix Array and PLCP Array
        auto& sa = t.require_sa(cm);
        auto& plcp = t.require_plcp(cm);

        const size_t n = t.size();

        // Compute LCP array
        this->env().begin_stat_phase("Construct LCP Array");

        m_max = plcp.max_lcp();
        const size_t w = bits_for(m_max);

        m_data = std::make_unique<iv_t>(n, 0,
            (cm == CompressMode::direct) ? w : LEN_BITS);

        (*m_data)[0] = 0;
		for(len_t i = 1; i < n; i++) {
            const len_t x = plcp[sa[i]];
			(*m_data)[i] = x;
		}

        if(cm == CompressMode::delayed) compress();

        this->env().log_stat("bit_width", size_t(m_data->width()));
        this->env().log_stat("size", m_data->bit_size() / 8);
        this->env().end_stat_phase();
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
