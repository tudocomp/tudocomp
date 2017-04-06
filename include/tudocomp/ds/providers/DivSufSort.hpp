#pragma once

#include <tudocomp/ds/TextDSProvider.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class DivSufSort : public TextDSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "divsufsort");
        return m;
    }

    using TextDSProvider::TextDSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t();
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { TextDS::SA };
    }
};

} //ns
