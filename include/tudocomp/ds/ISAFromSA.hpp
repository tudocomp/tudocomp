#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
class ISAFromSA : public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("isa", "from_sa");
        return m;
    }

    using ArrayDS::ArrayDS;

    template<typename textds_t>
    inline void construct(textds_t& t, CompressMode cm) {
        // Require Suffix Array
        auto& sa = t.require_sa(cm);

        env().begin_stat_phase("Construct ISA");

        // Allocate
        const size_t n = t.size();
        const size_t w = bits_for(n);
        m_data = std::make_unique<iv_t>(n, 0,
            (cm == CompressMode::direct) ? w : LEN_BITS);

        // Construct
        for(len_t i = 0; i < n; i++) {
            (*m_data)[sa[i]] = i;
        }

        if(cm == CompressMode::delayed) compress();

        env().log_stat("bit_width", size_t(m_data->width()));
        env().log_stat("size", m_data->bit_size() / 8);
        env().end_stat_phase();
    }

    void compress() {
        DCHECK(m_data);
        m_data->width(bits_for(m_data->size()));
        m_data->shrink_to_fit();
    }
};

} //ns
