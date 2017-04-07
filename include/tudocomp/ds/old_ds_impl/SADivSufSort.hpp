#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>
#include <tudocomp/util/divsufsort.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class SADivSufSort: public Algorithm, public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("sa", "divsufsort");
        return m;
    }

    template<typename textds_t>
    inline SADivSufSort(Env&& env, const textds_t& t, CompressMode cm)
        : Algorithm(std::move(env)) {

        StatPhase::wrap("Construct SA", [&]{
            // Allocate
            const size_t n = t.size();
            const size_t w = bits_for(n);

            // divsufsort needs one additional bit for signs
            set_array(iv_t(n, 0, (cm == CompressMode::compressed) ? w + 1 : LEN_BITS));

            // Use divsufsort to construct
            divsufsort(t.text(), (iv_t&) *this, n);

            StatPhase::log("bit_width", size_t(width()));
            StatPhase::log("size", bit_size() / 8);
        });

        if(cm == CompressMode::compressed || cm == CompressMode::delayed) {
            compress();
        }
    }

    void compress() {
        debug_check_array_is_initialized();

        StatPhase::wrap("Compress SA", [this]{
            width(bits_for(size()));
            shrink_to_fit();

            StatPhase::log("bit_width", size_t(width()));
            StatPhase::log("size", bit_size() / 8);
        });
    }
};

} //ns
