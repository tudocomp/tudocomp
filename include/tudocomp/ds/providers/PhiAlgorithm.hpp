#pragma once

#include <tudocomp/Algorithm.hpp>

namespace tdc {

/// Constructs the PLCP and LCP array using the Phi array.
class PhiAlgorithm : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "phi_algorithm");
        return m;
    }

    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::PLCP_ARRAY, ds::LCP_ARRAY>;
    using requires = std::index_sequence<ds::PHI_ARRAY>;
};

} //ns
