#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
class ISAFromSA: public Algorithm, public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("isa", "from_sa");
        return m;
    }

    template<typename textds_t>
    inline ISAFromSA(Env&& env, textds_t& t, CompressMode cm)
            : Algorithm(std::move(env)) {

        // Require Suffix Array
        auto& sa = t.require_sa(cm);

        StatPhase::wrap("Construct ISA", [&](StatPhase& phase) {
            // Allocate
            const size_t n = t.size();
            const size_t w = bits_for(n);
            set_array(iv_t(n, 0, (cm == CompressMode::compressed) ? w : LEN_BITS));

            // Construct
            for(len_t i = 0; i < n; i++) {
                (*this)[sa[i]] = i;
            }

            phase.log_stat("bit_width", size_t(width()));
            phase.log_stat("size", bit_size() / 8);
        });

        if(cm == CompressMode::delayed) compress();
    }

    void compress() {
        debug_check_array_is_initialized();

        StatPhase::wrap("Compress ISA", [this](StatPhase& phase) {
            width(bits_for(size()));
            shrink_to_fit();

            phase.log_stat("bit_width", size_t(width()));
            phase.log_stat("size", bit_size() / 8);
        });
    }
};

} //ns
