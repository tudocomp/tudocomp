#pragma once

#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
class ISASimple : public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("isa", "simple");
        return m;
    }

    using ArrayDS::ArrayDS;

    template<typename textds_t>
    inline void construct(textds_t& t) {
        // Require Suffix Array
        auto& sa = t.require_sa();

        env().begin_stat_phase("Construct ISA");

        // Allocate
        const size_t n = t.size();
        const size_t w = bits_for(n);
        m_data = iv_t(n, 0, w);

        // Construct
        for(len_t i = 0; i < n; i++) {
            m_data[sa[i]] = i;
        }

        env().end_stat_phase();
    }
};

} //ns
