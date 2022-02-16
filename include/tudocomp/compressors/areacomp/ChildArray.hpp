#pragma once

#include "tudocomp/ds/DSManager.hpp"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <tudocomp/compressors/areacomp/Consts.hpp>
#include <tudocomp/compressors/areacomp/areafunctions/AreaFunction.hpp>
#include <tudocomp/compressors/areacomp/areafunctions/ZeroArea.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/util/bits.hpp>
#include <vector>

namespace tdc::grammar::areacomp {

/**
 * @brief Implementation of lcp intervals and the child array according to "Optimal Exact String Matching Based on
 * Suffix Arrays" by Abouelhoda et al. (https://link.springer.com/chapter/10.1007/3-540-45735-6_4)
 *
 * @tparam lcp_arr_t The type of the lcp array
 */
template<typename LcpArray>
class ChildArray {

    // Attributes
    DynamicIntVector m_cld;
    LcpArray        &m_lcp_arr;

    size_t lcp(size_t i) const { return i == m_lcp_arr.size() ? 0 : m_lcp_arr[i]; }

    /**
     * @brief Calculates the "up" and "down" fields of the child table with Abouelhoda et al's Algorithm
     *
     * @param len The length of the lcp array
     * @param stack A stack used during calculation. This is expected to be empty starting out
     */
    void calculate_up_down(size_t len, std::vector<size_t> &stack) {
        int last_index = -1;
        stack.push_back(0);
        for (size_t i = 0; i <= len; i++) {
            size_t top = stack.back();
            while (lcp(i) < lcp(top)) {
                last_index = stack.back();
                stack.pop_back();
                top = stack.back();
                if (lcp(i) <= lcp(top) && lcp(top) != lcp(last_index)) {
                    m_cld[top] = last_index;
                }
            }

            if (lcp(i) >= lcp(top)) {
                if (last_index != -1) {
                    m_cld[i - 1] = last_index;
                    last_index   = -1;
                }
                stack.push_back(i);
            }
        }
    }

    /**
     * @brief Calculates the "nextLIndex" fields of the child table with Abouelhoda et al's Algorithm
     *
     * @param len The length of the lcp array
     * @param stack A stack used during calculation. This is expected to be empty starting out
     */
    void calculate_next_l_index(size_t len, std::vector<size_t> &stack) {
        stack.push_back(0);
        for (size_t i = 1; i <= len; i++) {
            while (lcp(i) < lcp(stack.back())) {
                stack.pop_back();
            }

            if (lcp(i) == lcp(stack.back())) {
                m_cld[stack.back()] = i;
                stack.pop_back();
            }
            stack.push_back(i);
        }
    }

  public:
    /**
     * @brief Return the first l-index of the lcp interval that ends at i - 1.
     *
     * Formally this is the lowest index q (lower than i) such that lcp[q] > lcp[i] and all lcp values between q and i
     * are at least as great as lcp[q].
     *
     * @param i The index after the last index of an lcp interval.
     *
     * @return The first l-index of the lcp interval ending at i - 1
     */
    size_t up(size_t i) const {
        if (i <= 0 || i >= cld_tab_len()) {
            return INVALID;
        }

        if (lcp(i - 1) > lcp(i)) {
            return m_cld[i - 1];
        }

        return INVALID;
    }

    /**
     * @brief Return the first l-index of the lcp interval starting at index i.
     *
     * Formally this is the largest index q (greater than i) such that lcp[q] > lcp[i] and all lcp values between i and
     * q are greater than lcp[q].
     *
     * @param i The start index of an lcp interval.
     *
     * @return The first l-index of the lcp interval starting at i.
     */
    size_t down(size_t i) const {
        if (i < 0 || i >= cld_tab_len()) {
            return INVALID;
        }

        if (lcp(i) < lcp(m_cld[i])) {
            return m_cld[i];
        }

        return INVALID;
    }

    /**
     * @brief Return the next l-index in the surrounding lcp interval.
     *
     * Formally this is the lowest index q (greater than i) such that lcp[q] = lcp[i] and all lcp values between q and i
     * are greater than lcp[q].
     *
     * @param i An l-index of an lcp-interval.
     *
     * @return The next l-index in the surrounding lcp interval.
     */
    size_t next_l_index(size_t i) const {
        if (i < 0 || i >= cld_tab_len() - 1) {
            return INVALID;
        }

        if (lcp(i) == lcp(m_cld[i])) {
            return m_cld[i];
        }

        return INVALID;
    }

    /**
     * @brief Creates a new child array
     *
     * @param lcp The lcp array
     * @param len The length of the lcp array
     */
    ChildArray(LcpArray &lcp, size_t len) : m_cld{len + 1, INVALID, bits_for(len + 1)}, m_lcp_arr{lcp} {

        std::vector<size_t> stack;
        m_cld.resize(len + 1, INVALID);
        calculate_up_down(len, stack);
        stack.clear();
        calculate_next_l_index(len, stack);

        m_cld.back() = 0;

        size_t max = *std::max_element(m_cld.begin(), m_cld.end());
        m_cld.width(bits_for(max));
        m_cld.shrink_to_fit();
    }

    /**
     * @brief Indexes the child array directly. This is not supposed to be used directly. It is advised to use the up,
     * down and next_l_index objects instead.
     *
     * @param i The index to be read from
     * @return size_t The value of the child array at index i.
     */
    size_t operator[](size_t i) const { return m_cld[i]; }

    /**
     * @brief Returns the lValue of a given lcp interval. Note that this function does not work for arbitrary intervals,
     * but only for lcp intervals as described in Abouelhoda et al's Paper
     *
     * @param low The lower bound of the lcp interval
     * @param high The upper bound of the lcp interval
     * @return The minimum value of the interval [low + 1, high] (inclusively) in lcp
     */
    size_t l_value(size_t low, size_t high) const {
        if (low >= high) {
            return 0;
        }

        if (low < up(high + 1) && up(high + 1) <= high) {
            return lcp(up(high + 1));
        } else {
            return lcp(down(low));
        }
    }

    /**
     * \brief Checks, whether there is another l_index of the same value after this one
     *
     * @param i The position to check
     * @return true, if there is another l_index with the same value as the one at index i. false, otherwise
     */
    bool has_next_l_index(size_t i) const {
        return m_cld[i] > i && lcp(i) == lcp(m_cld[i]) && next_l_index(i) != INVALID;
    }

  private:
    /**
     * Recursively add all the child intervals in the given lcp-interval (low, high) according to Abouelhoda et al with
     * a minimum l-value to a collection
     *
     * @param fun The area function to use
     * @param ds The text data structure provider that provides all necessary data stuctures for the area function
     * @param low The lower bound of the lcp interval
     * @param high The upper bound of the lcp interval
     * @param min_l_value The minimum l-value of the returned intervals
     * @param min_area_value The minimum area value of the returned intervals
     * @param start_list The collection to add the intervals' start positions into
     * @param len_list The collection to add the intervals' length into
     */
    template<typename Area, typename TextDS>
    requires tdc::grammar::areacomp::AreaFun<Area, TextDS, LcpArray>
    void add_child_intervals_with_area(Area             &fun,
                                       TextDS           &ds,
                                       size_t            low,
                                       size_t            high,
                                       size_t            min_area_value,
                                       size_t            min_l_value,
                                       DynamicIntVector &start_list,
                                       DynamicIntVector &len_list) const {

        if (low >= high)
            return;

        size_t current_l_index;

        // Get the first l index of the lcp interval
        if (low < up(high + 1) && up(high + 1) <= high) {
            current_l_index = up(high + 1);
        } else {
            current_l_index = down(low);
        }

        if (current_l_index - 1 > low && l_value(low, current_l_index - 1) >= min_l_value &&
            fun.area(ds, *this, low + 1, current_l_index - 1) >= min_area_value) {
            start_list.push_back(low);
            len_list.push_back(current_l_index - 1 - low);
        }
        add_child_intervals_with_area(fun,
                                      ds,
                                      low,
                                      current_l_index - 1,
                                      min_area_value,
                                      min_l_value,
                                      start_list,
                                      len_list);

        while (has_next_l_index(current_l_index)) {
            size_t next = next_l_index(current_l_index);
            if (next - 1 > current_l_index && l_value(current_l_index, next - 1) >= min_l_value &&
                fun.area(ds, *this, current_l_index + 1, next - 1) >= min_area_value) {
                start_list.push_back(current_l_index);
                len_list.push_back(next - 1 - current_l_index);
            }
            add_child_intervals_with_area(fun,
                                          ds,
                                          current_l_index,
                                          next - 1,
                                          min_area_value,
                                          min_l_value,
                                          start_list,
                                          len_list);
            current_l_index = next;
        }

        size_t l_val = l_value(current_l_index, high);

        if (high > current_l_index && l_val >= min_l_value && fun.area(ds, *this, current_l_index + 1, high)) {
            start_list.push_back(current_l_index);
            len_list.push_back(high - current_l_index);
        }
        add_child_intervals_with_area(fun,
                                      ds,
                                      current_l_index,
                                      high,
                                      min_area_value,
                                      min_l_value,
                                      start_list,
                                      len_list);
    }

  public:
    /**
     * @return The length of the child array -1. Which is the same as the length of the underlying lcp array.
     */
    const size_t cld_tab_len() const { return m_cld.size(); }

    /**
     * @brief Get all lcp intervals in this string with an l-value of at least min_l_value
     *
     * @param min_l_value The minimum l value of the lcp interval to include
     * @return A vector of lcp intervals with an l-value of at least min_l_value
     */
    std::pair<DynamicIntVector, DynamicIntVector> get_lcp_intervals(size_t min_l_value) const {
        auto area = Algorithm::instance<ZeroArea<DSManager<>(Config && cfg, const View &input)>>();
        auto ds   = Algorithm::instance<DSManager<>>();
        return get_lcp_intervals_with_area(*area, *ds, 0, min_l_value);
    }

    /**
     * @brief Get all lcp intervals in this string with an l-value of at least min_l_value and an area value of at
     * least min_area_value. This method takes an area function to evaluate the area of an interval.
     *
     * @tparam Area The type of the area function
     * @tparam TextDS The type of the text data structure provider
     * @param fun The area function to use
     * @param ds The text data structure provider that provides all necessary data stuctures for the area function
     * @param min_l_value The minimum l-value of the returned intervals
     * @param min_area_value The minimum area value of the returned intervals
     */
    template<typename Area, typename TextDS>
    requires AreaFun<Area, TextDS, LcpArray> std::pair<DynamicIntVector, DynamicIntVector>
    get_lcp_intervals_with_area(Area &fun, TextDS &ds, size_t min_area_value, size_t min_l_value)
    const {

        DynamicIntVector start_list;
        DynamicIntVector len_list;

        start_list.width(bits_for(cld_tab_len() - 1));
        len_list.width(bits_for(cld_tab_len() - 1));

        start_list.reserve(cld_tab_len());
        len_list.reserve(cld_tab_len());

        size_t last_l_index    = 1;
        size_t current_l_index = next_l_index(1);

        if (0 >= min_l_value && fun.area(ds, *this, 2, cld_tab_len() - 1) >= min_area_value) {
            start_list.push_back(1);
            len_list.push_back(cld_tab_len() - 1);
        }

        while (current_l_index != INVALID) {
            if (l_value(last_l_index, current_l_index - 1) >= min_l_value &&
                fun.area(ds, *this, last_l_index + 1, current_l_index - 1) >= min_area_value) {
                start_list.push_back(last_l_index);
                len_list.push_back(current_l_index - 1 - last_l_index);
            }
            add_child_intervals_with_area(fun,
                                          ds,
                                          last_l_index,
                                          current_l_index - 1,
                                          min_area_value,
                                          min_l_value,
                                          start_list,
                                          len_list);
            last_l_index    = current_l_index;
            current_l_index = next_l_index(current_l_index);
        }
        start_list.shrink_to_fit();
        len_list.shrink_to_fit();

        size_t max_start = 0;
        size_t max_len   = 0;

        for (size_t i = 0; i < start_list.size(); i++) {
            max_start = std::max(start_list[i] + 0, max_start);
            max_len   = std::max(len_list[i] + 0, max_len);
        }

        return {start_list, len_list};
    }

    /**
     * @brief The size of the text from which this child array has been created.
     *
     * This is 1 less than the size of the actual child array, since the child array simulates a sentinel that is larger
     * than all characters in the text.
     *
     * @return const size_t The size of the input text
     */
    const size_t input_len() const { return m_cld.size() - 1; }

    auto begin() { return m_cld.begin(); }

    auto end() { return m_cld.end(); }
};

} // namespace tdc::grammar::areacomp
