#pragma once

#include <tudocomp/ds/TextDSProvider.hpp>

namespace tdc {

/// Constructs the PLCP and LCP array using the Phi array.
class PhiAlgorithm : public TextDSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "phi_algorithm");
        return m;
    }

    using TextDSProvider::TextDSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t { TextDS::PHI };
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { TextDS::PLCP, TextDS::LCP };
    }
};

} //ns
