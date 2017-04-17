#pragma once

#include <tudocomp/ds/DSProvider.hpp>

namespace tdc {

/// Constructs the Phi array from the suffix array.
class PhiFromSA : public DSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "phi");
        return m;
    }

    using provides = tl::type_list<
        /*0 = SA   */ tl::None,
        /*1 = ISA  */ tl::None,
        /*2 = LCP  */ tl::None,
        /*3 = Phi  */ PhiFromSA,
        /*4 = PLCP */ tl::None
    >;

    using DSProvider::DSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t { ds::SUFFIX_ARRAY };
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { ds::PHI_ARRAY };
    }
};

} //ns
