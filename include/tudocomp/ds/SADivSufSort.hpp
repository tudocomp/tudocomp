#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>
#include <tudocomp/util/divsufsort.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class SADivSufSort : public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("sa", "divsufsort");
        return m;
    }

    using ArrayDS::ArrayDS;

    template<typename textds_t>
    inline void construct(const textds_t& t, CompressMode cm) {
        env().begin_stat_phase("Construct SA");

        // Allocate
        const size_t n = t.size();
        const size_t w = bits_for(n);

        // divsufsort needs one additional bit for signs
        m_data = std::make_unique<iv_t>(n, 0,
            (cm == CompressMode::direct) ? w + 1 : LEN_BITS);

        // Use divsufsort to construct
        divsufsort(t.text(), *m_data, n);

        if(cm == CompressMode::direct || cm == CompressMode::delayed) {
            compress();
        }

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
