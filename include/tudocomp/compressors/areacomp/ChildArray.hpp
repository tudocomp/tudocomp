#pragma once

#include <cstdio>
#include <vector>
#include <iostream>
#include <deque>

namespace tdc::grammar::areacomp {

/**
 * @brief Implementation of lcp intervals and the child array according to "Optimal Exact String Matching Based on Suffix Arrays" by Abouelhoda et al. (https://link.springer.com/chapter/10.1007/3-540-45735-6_4)
 *
 * @tparam lcp_arr_t The type of the lcp array
 */
template<typename lcp_arr_t>
class ChildArray {

    /**
     * @brief This class implements the up operation on the child array
     */
    class Up {

        ChildArray &cld;
        lcp_arr_t &lcp;

    public:
        Up(ChildArray &cld, lcp_arr_t &lcp) : cld{cld}, lcp{lcp} {}

        size_t operator[](size_t i) const {
            if (i <= 0 || i > cld.len()) {
                return INVALID;
            }

            if (lcp[i - 1] > lcp[i]) {
                return cld[i - 1];
            }

            return INVALID;
        }
    };

    /**
     * @brief This class implements the down operation on the child array
     */
    class Down {

        ChildArray &cld;
        lcp_arr_t &lcp;

    public:
        Down(ChildArray &cld, lcp_arr_t &lcp) : cld{cld}, lcp{lcp} {}

        size_t operator[](size_t i) const {
            if (i < 0 || i > cld.len()) {
                return INVALID;
            }

            if (lcp[i] < lcp[cld[i]]) {
                return cld[i];
            }

            return INVALID;
        }
    };

    /**
     * @brief This class implements the next_l_index operation on the child array
     */
    class NextLIndex {

        ChildArray &cld;
        lcp_arr_t &lcp;

    public:
        NextLIndex(ChildArray &cld, lcp_arr_t &lcp) : cld{cld}, lcp{lcp} {}

        size_t operator[](size_t i) const {
            if (i < 0 || i >= cld.len()) {
                return INVALID;
            }

            if (lcp[i] == lcp[cld[i]]) {
                return cld[i];
            }

            return INVALID;
        }
    };

    // Attributes

    std::vector<size_t> cld_tab;
    lcp_arr_t &lcp;

public:

    /**
     * @brief An object that when indexed with [] will return the result of the 'up' operation
     */
    const Up up;
    
    /**
     * @brief An object that when indexed with [] will return the result of the 'down' operation
     */
    const Down down;
    
    /**
     * @brief An object that when indexed with [] will return the next l index of the same value
     */
    const NextLIndex next_l_index;

    /**
     * @brief This value is returned if the return value from up, down or next_l_index is undefined
     */
    const static size_t INVALID = -1;

private:

    /**
     * @brief Calculates the "up" and "down" fields of the child table with Abouelhoda et al's Algorithm
     *
     * @param lcp_arr The lcp array for which to compute the child table
     * @param len The length of the lcp array
     * @param stack A stack used during calculation. This is expected to be empty starting out
     */
    void calculate_up_down(lcp_arr_t &lcp_arr, size_t len, std::deque<size_t> &stack) {
        auto lcp = [&](size_t i) -> size_t  {
            if(i == len) return 0;
            return lcp_arr[i] + 0; // workaround to have this lambda return a size_t
        };

        int last_index = -1;
        stack.push_front(0);
        for (size_t i = 0; i <= len; i++) {
            size_t top = stack.at(0);
            while (lcp(i) < lcp(top)) {
                last_index = stack.at(0);
                stack.pop_front();
                top = stack.at(0);
                if (lcp(i) <= lcp(top) && lcp(top) != lcp(last_index)) {
                    cld_tab[top] = last_index;
                }
            }

            if (lcp(i) >= lcp(top)) {
                if (last_index != -1) {
                    cld_tab[i - 1] = last_index;
                    last_index = -1;
                }
                stack.push_front(i);
            }
        }
        
    }

    /**
     * @brief Calculates the "nextLIndex" fields of the child table with Abouelhoda et al's Algorithm
     *
     * @param lcp_arr The lcp array for which to compute the child table
     * @param len The length of the lcp array
     * @param stack A stack used during calculation. This is expected to be empty starting out
     */
    void calculate_next_l_index(lcp_arr_t &lcp_arr, size_t len, std::deque<size_t> &stack) {
        auto lcp = [&](size_t i) -> size_t {
            if(i == len) return 0;
            return lcp_arr[i] + 0; // workaround to have this lambda return a size_t 
        };
        
        stack.push_front(0);
        for (size_t i = 1; i <= len; i++) {
            while (lcp(i) < lcp(stack.at(0))) {
                stack.pop_front();
            }

            if (lcp(i) == lcp(stack.at(0))) {
                cld_tab[stack.at(0)] = i;
                stack.pop_front();
            }
            stack.push_front(i);
        }
    }

public:

    struct Interval {
        const size_t start;
        const size_t end;

        Interval(size_t start, size_t end) : start{start}, end{end} {}

        explicit operator std::string() const {
            return "(" + std::to_string(start) + ", " + std::to_string(end) + ")";
        }
    };

    /**
     * @brief Creates a new child array
     *
     * @param lcp The lcp array
     * @param len The length of the lcp array
     */
    ChildArray(lcp_arr_t &lcp, size_t len) : 
        cld_tab{std::vector<size_t>(len, -1)},
        lcp{lcp},
        up{*this, lcp},
        down{*this, lcp},
        next_l_index{*this, lcp} {
        
        std::deque<size_t> stack;
        calculate_up_down(lcp, len, stack);
        stack.clear();
        calculate_next_l_index(lcp, len, stack);

        cld_tab.push_back(0);
    }

    /**
     * @brief Indexes the child array directly. This is not supposed to be used directly. It is advised to use the up, down and next_l_index objects instead. 
     * 
     * @param i The index to be read from
     * @return size_t The value of the child array at index i.
     */
    size_t operator[] (size_t i) const {
        return cld_tab[i];
    }

    /**
     * @brief Returns the lValue of a given lcp interval. Note that this function does not work for arbitrary intervals, but only
     * for lcp intervals as described in Abouelhoda et al's Paper
     *
     * @param low The lower bound of the lcp interval
     * @param high The upper bound of the lcp interval
     * @return The minimum value of the interval [low + 1, high] (inclusively) in lcp
     */
    size_t l_value(size_t low, size_t high) const {
        if(low >= high) {
            return 0;
        }

        if (low < up[high + 1] && up[high + 1] <= high) {
            return lcp[up[high + 1]];
        } else {
            return lcp[down[low]];
        }
    }

    /**
     * \brief Checks, whether there is another l_index of the same value after this one
     *
     * @param i The position to check
     * @return true, if there is another l_index with the same value as the one at index i. false, otherwise
     */
    bool has_next_l_index(size_t i) const {
        return cld_tab[i] > i && lcp[i] == lcp[cld_tab[i]] && next_l_index[i] != INVALID;
    }

private:
    /**
     * Recursively add all the child intervals in the given lcp-interval (low, high) according to Abouelhoda et al with a minimum l-value to a collection
     *
     * @param low The lower bound of the lcp interval
     * @param high The upper bound of the lcp interval
     * @param min_l_value The minimum l-value of the returned intervals
     * @param intervals The collection to add the intervals into
     */
    void add_child_intervals(size_t low, size_t high, size_t min_l_value, std::vector<Interval> &intervals) {
        if (low >= high) return;

        size_t current_l_index;

        // Get the first l index of the lcp interval
        if (low < up[high + 1] && up[high + 1] <= high) {
            current_l_index = up[high + 1];
        } else {
            current_l_index = down[low];
        }

        if (current_l_index - 1 > low && l_value(low, current_l_index - 1) >= min_l_value) {
            intervals.emplace_back(low, current_l_index - 1);
            add_child_intervals(low, current_l_index - 1, min_l_value, intervals);
        }

        while (has_next_l_index(current_l_index)) {
            size_t next = next_l_index[current_l_index];
            if (next - 1 > current_l_index && l_value(current_l_index, next - 1) >= min_l_value) {
                intervals.emplace_back(current_l_index, next - 1);
                add_child_intervals(current_l_index, next - 1, min_l_value, intervals);
            }
            current_l_index = next;
        }

        size_t l_val = l_value(current_l_index, high);

        if (high > current_l_index && l_val >= min_l_value) {
            intervals.emplace_back(current_l_index, high);
            add_child_intervals(current_l_index, high, min_l_value, intervals);
        }
    }


public:
    /**
     * @brief Get all lcp intervals in this string with an l-value of at least min_l_value
     *
     * @param min_l_value The minimum l value of the lcp interval to include
     * @return A vector of lcp intervals with an l-value of at least min_l_value
     */
    std::vector<Interval> get_lcp_intervals(size_t min_l_value) {
        std::vector<Interval> list;

        size_t last_l_index = 0;
        size_t current_l_index = next_l_index[0];

        if(0 >= min_l_value) list.push_back( {last_l_index, len()} );

        while (current_l_index != INVALID) {
            if (l_value(last_l_index, current_l_index - 1) >= min_l_value) {
                list.push_back( {last_l_index, current_l_index - 1} );
            }
            add_child_intervals(last_l_index, current_l_index - 1, min_l_value, list);
            last_l_index = current_l_index;
            current_l_index = next_l_index[current_l_index];
        }

        return list;
    }

    /**
     * @return The length of the child array -1. Which is the same as the length of the lcp array.
     */
    const size_t len() const {
        return cld_tab.size() - 1;
    }

    auto begin() {
        return cld_tab.begin();
    }

    auto end() {
        return cld_tab.end();
    }

};


}