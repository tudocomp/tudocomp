#pragma once

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
    inline void construct(const textds_t& t) {
        env().begin_stat_phase("Construct SA");

        // Allocate
        const size_t n = t.size();
        const size_t w = bits_for(n);

        //divsufsort needs one additional bit for signs
        m_data = std::make_unique<iv_t>(n, 0, w + 1);

        // Use divsufsort to construct
        divsufsort(t.text(), *m_data, n);

        // Shrink to required width
        m_data->width(w);

        env().end_stat_phase();
    }
};

} //ns
