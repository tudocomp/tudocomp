#pragma once

#include <tudocomp/ds/DSProvider.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class DivSufSort : public DSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "divsufsort");
        return m;
    }

    using DSProvider::DSProvider;

    using provides = std::index_sequence<ds::SUFFIX_ARRAY>;
    using requires = std::index_sequence<>;
};

} //ns
