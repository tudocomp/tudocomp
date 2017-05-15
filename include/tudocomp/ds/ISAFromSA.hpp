#pragma once

#include <tudocomp/ds/SparseISA.hpp>

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
template<typename textds_t>
class ISAFromSA {
private:
    SparseISA<textds_t> isa;
public:
    inline static Meta meta() {
        Meta m("isa", "from_sa");
        return m;
    }

    inline ISAFromSA(Env&& env, textds_t& t, CompressMode cm) :
            isa(std::move(env), t, cm) {
    }

    inline int operator[](const size_t i) const {
        return isa.getInv(i);
    }
};

} //ns
