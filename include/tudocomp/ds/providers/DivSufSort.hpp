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

    using provides = tl::set<ds::SUFFIX_ARRAY, DivSufSort>;

    using DSProvider::DSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t();
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { ds::SUFFIX_ARRAY };
    }
};

} //ns
