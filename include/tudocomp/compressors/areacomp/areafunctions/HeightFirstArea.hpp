#pragma once

#include <tudocomp/compressors/areacomp/ChildArray.hpp>
#include <tudocomp/compressors/areacomp/areafunctions/AreaFunction.hpp>

namespace tdc::grammar::areacomp {

template<typename ds_t>
struct HeightFirstArea : public Algorithm {

    inline static Meta meta() {
        Meta m(area_fun_type_desc(), "height_first", "Always prioritizes the lcp interval with the greatest l value");
        return m;
    }

    using Algorithm::Algorithm; // import constructor

    len_t area(ds_t &text_ds, ChildArray<const DynamicIntVector> &cld, size_t low, size_t high) {
        auto lcp_len = static_cast<len_t>(cld.l_value(low - 1, high));
        return lcp_len;
    };
};

} // namespace tdc::grammar::areacomp
