#pragma once

#include <tudocomp/compressors/areacomp/areafunctions/AreaData.hpp>
#include <tudocomp/compressors/areacomp/ChildArray.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc::grammar::areacomp {

template<typename ds_t>
struct AreaFunction : public Algorithm {
    static TypeDesc type_desc() {
        return {"area_function"};
    }

    using Algorithm::Algorithm; //import constructor


    virtual AreaData area(ds_t &text_ds, ChildArray<const DynamicIntVector> &cld, size_t low, size_t high) = 0;
};

/**
 * @brief Calculates an area value for a given interval in the lcp array
 *
 * @param text_ds The text data structure provider, providing the suffix array, lcp array etc.
 * @param cld The child array according to Abouelhoda et al.
 * @param low The start index of the lcp interval (inclusive)
 * @param high The end of the lcp interval (exclusive)
 * @return AreaData an object containing the results of the calculation
 */
template<typename area_fun_t, typename ds_t>
concept AreaFun = requires(area_fun_t fun, ds_t ds_manager, ChildArray<const DynamicIntVector> cld, size_t low, size_t high) {
    { fun.area(ds_manager, cld, low, high) } -> std::convertible_to<AreaData>;
};



} // namespace tdc
