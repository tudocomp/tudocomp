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

    using provides = tdc::type_list::type_list<
        /*0 = SA   */ DivSufSort,
        /*1 = ISA  */ tdc::type_list::None,
        /*2 = LCP  */ tdc::type_list::None,
        /*3 = Phi  */ tdc::type_list::None,
        /*4 = PLCP */ tdc::type_list::None
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
