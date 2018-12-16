#pragma once

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the PLCP array using the phi array.
class PLCPFromPhi: public Algorithm, public ArrayDS {
private:
    len_t m_max;

public:
    inline static Meta meta() {
        Meta m(TypeDesc("plcp"), "from_phi",
            "Constructs the PLCP array using the phi array.");
        return m;
    }

    template<typename textds_t>
    inline PLCPFromPhi(Config&& cfg, textds_t& t, CompressMode cm)
            : Algorithm(std::move(cfg)) {

        const size_t n = t.size();

        // Construct Phi and attempt to work in-place
        set_array(t.inplace_phi(cm));

        StatPhase::wrap("Construct PLCP Array", [&]{
            // Use Phi algorithm to compute PLCP array
            m_max = 0;
            for(len_t i = 0, l = 0; i < n - 1; ++i) {
                const len_t phii = (*this)[i];
                while(t[i+l] == t[phii+l]) ++l;
                m_max = std::max(m_max, l);
                (*this)[i] = l;
                if(l) --l;
            }

            StatPhase::log("bit_width", size_t(width()));
            StatPhase::log("size", bit_size() / 8);
        });

        if(cm == CompressMode::compressed || cm == CompressMode::delayed) {
            compress();
        }
    }

	inline len_t max_lcp() const {
		return m_max;
	}

    void compress() {
        debug_check_array_is_initialized();

        StatPhase::wrap("Compress PLCP Array", [this]{
            width(bits_for(m_max));
            shrink_to_fit();

            StatPhase::log("bit_width", size_t(width()));
            StatPhase::log("size", bit_size() / 8);
        });
    }
};

} //ns
