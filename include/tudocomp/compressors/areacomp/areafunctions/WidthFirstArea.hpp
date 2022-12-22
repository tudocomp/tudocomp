#pragma once

#include "tudocomp/Algorithm.hpp"
#include <tudocomp/compressors/areacomp/ChildArray.hpp>
#include <tudocomp/compressors/areacomp/areafunctions/AreaFunction.hpp>

namespace tdc::grammar::areacomp {

template<typename ds_t>
struct WidthFirstArea : public Algorithm {

    inline static Meta meta() {
        Meta m(area_fun_type_desc(), "width_first", "Always prioritizes the lcp interval with the greatest width");
        return m;
    }

    using Algorithm::Algorithm; // import constructor

    len_t area(ds_t &text_ds, const ChildArray<const DynamicIntVector> &cld, size_t low, size_t high) {
        return high - low + 1;
    };
};

} // namespace tdc::grammar::areacomp
