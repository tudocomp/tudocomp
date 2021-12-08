#pragma once

#include <tudocomp/compressors/areacomp/areafunctions/AreaData.hpp>
#include <tudocomp/compressors/areacomp/ChildArray.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSManager.hpp>

namespace tdc::grammar::areacomp {

template<typename ds_t=DSManager<>>
struct AreaFunction : public Algorithm { 
    static TypeDesc type_desc() {
        return TypeDesc("area_function");
    }
 
    using Algorithm::Algorithm; //import constructor

    /**
     * @brief Calculates an area value for a given interval in the lcp array
     * 
     * @param text_ds The text data structure provider, providing the suffix array, lcp array etc.
     * @param cld The child array according to Abouelhoda et al.
     * @param low The start index of the lcp interval (inclusive)
     * @param high The end of the lcp interval (exclusive)
     * @return AreaData an object containing the results of the calculation
     */
    virtual AreaData area(ds_t &text_ds, ChildArray<const DynamicIntVector> &cld, size_t low, size_t high) = 0;
};




} // namespace tdc
