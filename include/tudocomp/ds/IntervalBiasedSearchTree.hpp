#pragma once

#include "tudocomp/ds/Rank.hpp"
#include "tudocomp/ds/Select.hpp"
#include "tudocomp/util/bits.hpp"
#include <algorithm>
#include <concepts>
#include <iostream>
#include <ranges>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

template<typename T>
concept CollectionSize = requires(T a) {
    { a.size() } -> std::convertible_to<std::size_t>;
};

template<typename IntType>
requires std::integral<IntType>
class IntervalBiasedSearchTree {

    size_t m_size;
    /**
     * @brief Stores the Balanced Parantheses Sequence of this tree. There are probably better ways to do this for
     * binary trees.
     */
    BitVector m_parentheses;
    /**
     * @brief Stores whether the current node is a left or right child. This is needed if a node has only one child as
     * it would be ambiguous in the BPS.
     * Here: 0 is a left child and 1 is a right child.
     * The values are stored in preorder.
     */
    BitVector        m_structure;
    DynamicIntVector m_interval_content;

    Rank    m_rank_parentheses;
    Select1 m_select_parentheses;

  private:
    template<typename Range>
    requires std::ranges::random_access_range<Range> && CollectionSize<Range>
    void construct_recursive(const Range &it, size_t lo, size_t hi) {

        // Narrow search space
        const size_t avg = (it[lo] + it[hi - 1]) >> 1;

        size_t offset    = 2;
        size_t search_lo = lo;
        while (search_lo + offset < hi && it[search_lo + offset] <= avg) {
            search_lo += offset;
            offset <<= 1;
        }

        offset           = 2;
        size_t search_hi = hi;
        while (search_hi >= lo + offset && it[search_hi - offset] >= avg) {
            search_hi -= offset;
            offset <<= 1;
        }

        // Binary Search on the narrowed search space
        while (search_lo <= search_hi) {
            const size_t mid = (search_lo + search_hi) >> 1;
            if (it[mid] <= avg && it[mid + 1] > avg) {
                // Search completed. Continue recursively
                std::cout << "Inserting: (" << it[mid] << ", " << it[mid + 1] << ")" << std::endl;
                m_parentheses.push_back(1);
                m_interval_content.push_back(it[mid]);
                m_interval_content.push_back(it[mid + 1]);
                if (mid + 1 - lo >= 2) {
                    m_structure.push_back(0);
                    construct_recursive(it, lo, mid + 1);
                }
                if (hi - mid - 1 >= 2) {
                    m_structure.push_back(1);
                    construct_recursive(it, mid + 1, hi);
                }
                m_parentheses.push_back(0);
                return;
            } else if (it[mid] <= avg) {
                search_lo = mid + 1;
            } else {
                search_hi = mid - 1;
            }
        }
    }

  public:
    template<typename Range>
    requires std::ranges::random_access_range<Range> && CollectionSize<Range>
    inline IntervalBiasedSearchTree(const Range &it) :
        m_size{it.size() - 1},
        m_parentheses{},
        m_structure{},
        m_interval_content{} {

        m_parentheses.reserve(2 * m_size);
        m_interval_content.width(bits_for(it[it.size() - 1] + 1));
        m_interval_content.reserve(2 * m_size);
        m_structure.reserve(m_size);
        // Doesn't really matter which value goes in here. We just need to put a dummy value in here for the node, so
        // the calculations match up
        m_structure.push_back(0);
        construct_recursive(it, 0, it.size());

        m_rank_parentheses   = Rank(m_parentheses);
        m_select_parentheses = Select1(m_parentheses);
    }

    const DynamicIntVector &interval_data() const { return m_interval_content; }

    const BitVector &parentheses() const { return m_parentheses; }

    const BitVector &structure() const { return m_structure; }

    /*const size_t predecessor(size_t value) {
        // 0 is the index of the root
        auto current = 0;

        while (true) {

            auto l = m_interval_content[2 * current];
            auto r = m_interval_content[2 * current + 1];

            if (l <= value && value < r) {
                return l;
            } else if (value == r) {
                return r;
            } else {
            }
        }
    }*/
};

} // namespace tdc
