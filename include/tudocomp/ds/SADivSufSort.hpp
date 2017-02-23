#pragma once

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>
#include <tudocomp/util/divsufsort.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class SADivSufSort: public Algorithm, public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("sa", "divsufsort");
        return m;
    }

    inline static ds::InputRestriction restrictions() {
        return ds::InputRestriction {
            { 0 },
            true
        };
    }

    template<typename textds_t>
    inline SADivSufSort(Env&& env, const textds_t& t, CompressMode cm)
        : Algorithm(std::move(env)) {

        this->env().begin_stat_phase("Construct SA");

        // Allocate
        const size_t n = t.size();
        const size_t w = bits_for(n);

        // divsufsort needs one additional bit for signs
        set_array(iv_t(n, 0, (cm == CompressMode::compressed) ? w + 1 : LEN_BITS));

        // Use divsufsort to construct
        divsufsort(t.text(), (iv_t&) *this, n);

        this->env().log_stat("bit_width", size_t(width()));
        this->env().log_stat("size", bit_size() / 8);
        this->env().end_stat_phase();

        if(cm == CompressMode::compressed || cm == CompressMode::delayed) {
            compress();
        }
    }

    void compress() {
        debug_check_array_is_initialized();

        env().begin_stat_phase("Compress SA");

        width(bits_for(size()));
        shrink_to_fit();

        env().log_stat("bit_width", size_t(width()));
        env().log_stat("size", bit_size() / 8);
        env().end_stat_phase();
    }
};

} //ns
