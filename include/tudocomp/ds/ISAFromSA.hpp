#pragma once

#include <tudocomp/ds/SparseISA.hpp>

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
class ISAFromSA : public SparseISA {
private:
public:
    inline static Meta meta() {
        Meta m("isa", "from_sa");
        return m;
    }

    template<typename textds_t>
    inline ISAFromSA(Env&& env, textds_t& t, CompressMode cm) : SparseISA(std::move(env),t,cm) {
    }

    inline int operator[](size_t i) const {
        return getInv(i);
    }
};

} //ns
