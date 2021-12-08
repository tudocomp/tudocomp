#pragma once

#include <tudocomp/compressors/areacomp/areafunctions/AreaFunction.hpp>
#include <tudocomp/compressors/areacomp/ChildArray.hpp>

namespace tdc::grammar::areacomp {

    template<typename ds_t>
    struct SimpleArea : public AreaFunction<ds_t> {

        inline static Meta meta() {
            Meta m(
                    AreaFunction<>::type_desc(),
                    "simple",
                    "Calculates the area as the product of the width and height"
            );
            return m;
        }

        using AreaFunction<ds_t>::AreaFunction; //import constructor

        AreaData area(ds_t &text_ds, ChildArray<const DynamicIntVector> &cld, size_t low, size_t high) {
            auto lcp_len = cld.l_value(low - 1, high);
            return {low, high, lcp_len * (high - low + 1), lcp_len};
        };
    };

}

