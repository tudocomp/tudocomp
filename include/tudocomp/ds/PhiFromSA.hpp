#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the Phi array using the suffix array.
class PhiFromSA: public Algorithm, public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("phi", "from_sa");
        return m;
    }

    template<typename textds_t>
    inline PhiFromSA(Env&& env, textds_t& t, CompressMode cm)
            : Algorithm(std::move(env)) {

        // Construct Suffix Array
        auto& sa = t.require_sa(cm);

        const size_t n = t.size();
        const size_t w = bits_for(n);

        StatPhase::wrap("Construct Phi Array", [&](StatPhase& phase) {
            // Construct Phi Array
            set_array(iv_t(n, 0, (cm == CompressMode::compressed) ? w : LEN_BITS));

            for(len_t i = 1, prev = sa[0]; i < n; i++) {
                (*this)[sa[i]] = prev;
                prev = sa[i];
            }
            (*this)[sa[0]] = sa[n-1];

            phase.log_stat("bit_width", size_t(width()));
            phase.log_stat("size", bit_size() / 8);
        });

        if(cm == CompressMode::delayed) compress();
    }

    void compress() {
        debug_check_array_is_initialized();

        StatPhase::wrap("Compress Phi Array", [this](StatPhase& phase) {
            width(bits_for(size()));
            shrink_to_fit();

            phase.log_stat("bit_width", size_t(width()));
            phase.log_stat("size", bit_size() / 8);
        });
    }
};

} //ns
