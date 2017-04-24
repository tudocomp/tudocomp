#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class DivSufSort : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "divsufsort");
        return m;
    }

    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::SUFFIX_ARRAY>;
    using requires = std::index_sequence<>;
};

} //ns
