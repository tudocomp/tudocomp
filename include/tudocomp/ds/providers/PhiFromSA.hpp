#pragma once

#include <tudocomp/ds/TextDSProvider.hpp>

namespace tdc {

/// Constructs the Phi array from the suffix array.
class PhiFromSA : public TextDSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "phi");
        return m;
    }

    using TextDSProvider::TextDSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t { TextDS::SA };
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { TextDS::PHI };
    }
};

} //ns
