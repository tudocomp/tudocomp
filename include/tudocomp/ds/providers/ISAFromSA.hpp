#pragma once

#include <tudocomp/ds/DSProvider.hpp>

namespace tdc {

/// Constructs the inverse suffix array from the suffix array.
class ISAFromSA : public DSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "isa");
        return m;
    }

    using DSProvider::DSProvider;

    using provides = std::index_sequence<ds::INVERSE_SUFFIX_ARRAY>;
    using requires = std::index_sequence<ds::SUFFIX_ARRAY>;
};

} //ns
