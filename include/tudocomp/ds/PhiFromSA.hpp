#pragma once

#include <tudocomp/ds/ArrayDS.hpp>

namespace tdc {

/// Constructs the Phi array using the suffix array.
class PhiFromSA : public ArrayDS {
public:
    inline static Meta meta() {
        Meta m("phi", "from_sa");
        return m;
    }

    using ArrayDS::ArrayDS;

    template<typename textds_t>
    inline void construct(textds_t& t) {
        // Construct Suffix Array
        auto& sa = t.require_sa();

        const size_t n = t.size();
        const size_t w = bits_for(n);

        // Construct Phi Array
        env().begin_stat_phase("Construct Phi Array");

        m_data = std::make_unique<iv_t>(n, 0, w);

        for(len_t i = 1, prev = sa[0]; i < n; i++) {
            (*m_data)[sa[i]] = prev;
            prev = sa[i];
        }
        (*m_data)[sa[0]] = sa[n-1];

        env().end_stat_phase();
    }

};

} //ns
