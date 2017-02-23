#pragma once

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the LCP array using the Phi algorithm.
class LCPFromPLCP: public Algorithm, public ArrayDS {
private:
    len_t m_max;

public:
    inline static Meta meta() {
        Meta m("lcp", "from_phi");
        return m;
    }

    inline static ds::InputRestriction restrictions() {
        return ds::InputRestriction {};
    }

    template<typename textds_t>
    inline LCPFromPLCP(Env&& env, textds_t& t, CompressMode cm)
            : Algorithm(std::move(env)) {

        // Construct Suffix Array and PLCP Array
        auto& sa = t.require_sa(cm);
        auto& plcp = t.require_plcp(cm);

        const size_t n = t.size();

        // Compute LCP array
        this->env().begin_stat_phase("Construct LCP Array");

        m_max = plcp.max_lcp();
        const size_t w = bits_for(m_max);

        set_array(iv_t(n, 0, (cm == CompressMode::compressed) ? w : LEN_BITS));

        (*this)[0] = 0;
		for(len_t i = 1; i < n; i++) {
            const len_t x = plcp[sa[i]];
			(*this)[i] = x;
		}

        this->env().log_stat("bit_width", size_t(width()));
        this->env().log_stat("size", bit_size() / 8);
        this->env().end_stat_phase();

        if(cm == CompressMode::delayed) compress();
    }

	inline len_t max_lcp() const {
		return m_max;
	}

    void compress() {
        debug_check_array_is_initialized();

        env().begin_stat_phase("Compress LCP Array");

        width(bits_for(m_max));
        shrink_to_fit();

        env().log_stat("bit_width", size_t(width()));
        env().log_stat("size", bit_size() / 8);
        env().end_stat_phase();
    }
};

} //ns
