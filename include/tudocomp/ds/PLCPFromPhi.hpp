#pragma once

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the PLCP array using the phi array.
class PLCPFromPhi: public Algorithm, public ArrayDS {
private:
    len_t m_max;

public:
    inline static Meta meta() {
        Meta m("plcp", "from_phi");
        return m;
    }

    inline static ds::InputRestriction restrictions() {
        return ds::InputRestriction {};
    }

    template<typename textds_t>
    inline PLCPFromPhi(Env&& env, textds_t& t, CompressMode cm)
            : Algorithm(std::move(env)) {

        const size_t n = t.size();

        // Construct Phi and attempt to work in-place
        set_array(t.inplace_phi(cm));

        // Use Phi algorithm to compute PLCP array
        this->env().begin_stat_phase("Construct PLCP Array");

        m_max = 0;
        for(len_t i = 0, l = 0; i < n - 1; ++i) {
			const len_t phii = (*this)[i];
			while(t[i+l] == t[phii+l]) ++l;
            m_max = std::max(m_max, l);
			(*this)[i] = l;
			if(l) --l;
		}

        this->env().log_stat("bit_width", size_t(width()));
        this->env().log_stat("size", bit_size() / 8);
        this->env().end_stat_phase();

        if(cm == CompressMode::compressed || cm == CompressMode::delayed) {
            compress();
        }
    }

	inline len_t max_lcp() const {
		return m_max;
	}

    void compress() {
        debug_check_array_is_initialized();

        env().begin_stat_phase("Compress PLCP Array");

        width(bits_for(m_max));
        shrink_to_fit();

        env().log_stat("bit_width", size_t(width()));
        env().log_stat("size", bit_size() / 8);
        env().end_stat_phase();
    }
};

} //ns
