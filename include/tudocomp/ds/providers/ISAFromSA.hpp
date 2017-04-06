#pragma once

#include <tudocomp/ds/TextDSProvider.hpp>

namespace tdc {

/// Constructs the inverse suffix array from the suffix array.
class ISAFromSA : public TextDSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "isa");
        return m;
    }

    using TextDSProvider::TextDSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t { TextDS::SA };
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { TextDS::ISA };
    }
};

} //ns
