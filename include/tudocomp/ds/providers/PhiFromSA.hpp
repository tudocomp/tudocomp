#pragma once

#include <tudocomp/Algorithm.hpp>

namespace tdc {

/// Constructs the Phi array from the suffix array.
class PhiFromSA : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "phi");
        return m;
    }

    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::PHI_ARRAY>;
    using requires = std::index_sequence<ds::SUFFIX_ARRAY>;
};

} //ns
