#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the Phi array using the suffix array.
class PhiFromSA : public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("phi", "from_sa");
        return m;
    }

    template<typename textds_t>
    inline PhiFromSA(Env&& env, textds_t& t, CompressMode cm)
            : ArrayDS(std::move(env)) {

        // Construct Suffix Array
        auto& sa = t.require_sa(cm);

        const size_t n = t.size();
        const size_t w = bits_for(n);

        // Construct Phi Array
        this->env().begin_stat_phase("Construct Phi Array");

        m_data = std::make_unique<iv_t>(n, 0,
            (cm == CompressMode::compressed) ? w : LEN_BITS);

        for(len_t i = 1, prev = sa[0]; i < n; i++) {
            (*m_data)[sa[i]] = prev;
            prev = sa[i];
        }
        (*m_data)[sa[0]] = sa[n-1];

        this->env().log_stat("bit_width", size_t(m_data->width()));
        this->env().log_stat("size", m_data->bit_size() / 8);
        this->env().end_stat_phase();

        if(cm == CompressMode::delayed) compress();
    }

    void compress() {
        DCHECK(m_data);

        env().begin_stat_phase("Compress Phi Array");

        m_data->width(bits_for(m_data->size()));
        m_data->shrink_to_fit();

        env().log_stat("bit_width", size_t(m_data->width()));
        env().log_stat("size", m_data->bit_size() / 8);
        env().end_stat_phase();
    }
};

} //ns
