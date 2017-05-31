#pragma once

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the LCP array using the Phi algorithm.
class LCPFromPLCP: public Algorithm, public ArrayDS {
private:
    index_fast_t m_max;

public:
    inline static Meta meta() {
        Meta m("lcp", "from_phi");
        return m;
    }

    inline static ds::InputRestrictions restrictions() {
        return ds::InputRestrictions {};
    }

    template<typename textds_t>
    inline LCPFromPLCP(Env&& env, textds_t& t, CompressMode cm)
            : Algorithm(std::move(env)) {

        // Construct Suffix Array and PLCP Array
        auto& sa = t.require_sa(cm);
        auto& plcp = t.require_plcp(cm);

        const size_t n = t.size();

        StatPhase::wrap("Construct LCP Array", [&]{
            // Compute LCP array
            m_max = plcp.max_lcp();
            const size_t w = bits_for(m_max);

            set_array(iv_t(n, 0, (cm == CompressMode::compressed) ? w : INDEX_FAST_BITS));

            (*this)[0] = 0;
            for(index_fast_t i = 1; i < n; i++) {
                const index_fast_t x = plcp[sa[i]];
                (*this)[i] = x;
            }

            StatPhase::log("bit_width", size_t(width()));
            StatPhase::log("size", bit_size() / 8);
        });

        if(cm == CompressMode::delayed) compress();
    }

	inline index_fast_t max_lcp() const {
		return m_max;
	}

    void compress() {
        debug_check_array_is_initialized();

        StatPhase::wrap("Compress LCP Array", [this]{
            width(bits_for(m_max));
            shrink_to_fit();

            StatPhase::log("bit_width", size_t(width()));
            StatPhase::log("size", bit_size() / 8);
        });
    }
};

} //ns
