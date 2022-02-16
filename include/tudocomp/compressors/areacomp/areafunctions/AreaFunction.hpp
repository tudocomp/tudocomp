#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc::grammar::areacomp {

static TypeDesc area_fun_type_desc() { return {"area_function"}; }
template<typename lcp_arr_t = const DynamicIntVector>
class ChildArray;

/**
 * @brief Calculates an area value for a given interval in the lcp array
 *
 * @param text_ds The text data structure provider, providing the suffix array, lcp array etc.
 * @param cld The child array according to Abouelhoda et al.
 * @param low The start index of the lcp interval (inclusive)
 * @param high The end of the lcp interval (exclusive)
 * @return AreaData an object containing the results of the calculation
 */
template<typename area_fun_t, typename ds_t, typename lcp_arr_t = const DynamicIntVector>
concept AreaFun = requires(area_fun_t fun, ds_t ds_manager, const ChildArray<lcp_arr_t> &cld, size_t low, size_t high) {
    { fun.area(ds_manager, cld, low, high) } -> std::convertible_to<len_t>;
};

} // namespace tdc::grammar::areacomp
