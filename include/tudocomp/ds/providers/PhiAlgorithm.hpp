#pragma once

#include <tudocomp/ds/DSProvider.hpp>

namespace tdc {

/// Constructs the PLCP and LCP array using the Phi array.
class PhiAlgorithm : public DSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "phi_algorithm");
        return m;
    }

    using provides = tl::type_list<
        /*0 = SA   */ tl::None,
        /*1 = ISA  */ tl::None,
        /*2 = LCP  */ PhiAlgorithm,
        /*3 = Phi  */ tl::None,
        /*4 = PLCP */ PhiAlgorithm
    >;

    using DSProvider::DSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t { ds::PHI_ARRAY };
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { ds::PLCP_ARRAY, ds::LCP_ARRAY };
    }
};

} //ns
