#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/areacomp/areafunctions/AreaFunction.hpp>
#include <tudocomp/ds/DSManager.hpp>
#include <tudocomp/ds/IntVector.hpp>
namespace tdc::grammar::areacomp {

template<typename ds_t = DSManager<>>
class ZeroArea : public tdc::Algorithm {

    inline static Meta meta() {
        Meta m(area_fun_type_desc(),
               "zero",
               "Always returns zero. A dummy area for cases in which it is required. Using this area for AreaComp will "
               "result in an arbitrary order of lcp intervals.");
        return m;
    }

    using Algorithm::Algorithm; // import constructor

    len_t area(ds_t &text_ds, const ChildArray<const DynamicIntVector> &cld, size_t low, size_t high) { return 0; };
};
} // namespace tdc::grammar::areacomp
