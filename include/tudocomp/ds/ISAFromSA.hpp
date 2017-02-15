#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

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

        this->env().begin_stat_phase("Construct ISA");

        // Allocate
        const size_t n = t.size();
        const size_t w = bits_for(n);
        set_array(iv_t(n, 0, (cm == CompressMode::compressed) ? w : LEN_BITS));

        // Construct
        for(len_t i = 0; i < n; i++) {
            (*this)[sa[i]] = i;
        }

        this->env().log_stat("bit_width", size_t(width()));
        this->env().log_stat("size", bit_size() / 8);
        this->env().end_stat_phase();

        if(cm == CompressMode::delayed) compress();
    }

    void compress() {
        debug_check_array_is_initialized();

        env().begin_stat_phase("Compress ISA");

        width(bits_for(size()));
        shrink_to_fit();

        env().log_stat("bit_width", size_t(width()));
        env().log_stat("size", bit_size() / 8);
        env().end_stat_phase();
    }
};

} //ns
