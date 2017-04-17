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

    using provides = tl::type_list<
        /*0 = SA   */ DivSufSort,
        /*1 = ISA  */ tl::None,
        /*2 = LCP  */ tl::None,
        /*3 = Phi  */ tl::None,
        /*4 = PLCP */ tl::None
    >;

    using DSProvider::DSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t();
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { ds::SUFFIX_ARRAY };
    }
};

} //ns
