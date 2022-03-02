#pragma once

#include "tudocomp/util/GenericView.hpp"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <limits>
#include <optional>
#include <tdc/pred/dynamic/buckets/buckets.hpp>
#include <tdc/pred/dynamic/dynamic_index.hpp>
#include <tudocomp/compressors/areacomp/Consts.hpp>
#include <tudocomp/def.hpp>

namespace tdc::grammar::areacomp {

/**
 * @brief A data structure keeping track of replaced subsequences in a text by rules of a grammar.
 *
 * @tparam sampling The sampling parameter of the underlying dynamic index predecessor data structure.
 */
template<uint8_t sampling = 16>
class RuleIntervalIndex;

/**
 * @brief A class storing information about a subsequence of text that has been replaced by the occurrence of the right
 * side of a rule in a grammar.
 */
class RuleInterval {

    /**
     * @brief The id of the rule which this rule interval is an instance of
     */
    const len_t m_rule_id;

    /**
     * @brief The start index of this interval in the input text
     */
    const len_t m_start;

    /**
     * @brief The index of this interval's parent in the rule interval index' interval list
     */
    len_t m_parent_index;

    /**
     * @brief The index in the rule interval index' interval list of the least deeply nested interval that starts at the
     * same index
     */
    len_t m_first_at_start_index;

    /**
     * @brief The index in the rule interval index' interval list of the next more deeply nested interval that starts at
     * the same index
     */
    len_t m_next_at_start_index;

  public:
    /**
     * @brief Creates a new rule interval with the given rule id at the given start position.
     *
     * All other values are set to @link INVALID.
     *
     * @param rule_id The rule id
     * @param start The start index in the text
     */
    RuleInterval(len_t rule_id, len_t start) :
        m_rule_id{rule_id},
        m_start{start},
        m_parent_index{INVALID},
        m_first_at_start_index{INVALID},
        m_next_at_start_index{INVALID} {}

    /**
     * @brief Inserts a new parent into this interval.
     *
     * @param this_index This interval's index in the interval list vector of the rule interval index data structure.
     * @param new_parent_index The to-be-inserted parent's index in the interval list vector of the rule interval index
     * data structure.
     * @param interval_list The rule interval index' interval list.
     */
    void insert_parent(len_t this_index, len_t new_parent_index, std::vector<RuleInterval> &interval_list) {
        auto &new_parent = interval_list[new_parent_index];
        if (has_parent()) {
            auto &parent = interval_list[m_parent_index];

            new_parent.m_parent_index = this->m_parent_index;
            if (parent.m_start == new_parent.m_start) {
                new_parent.m_first_at_start_index = parent.m_first_at_start_index;
                parent.m_next_at_start_index = new_parent_index;
            }
        }
        this->m_parent_index = new_parent_index;
        if (new_parent.m_start == this->m_start) {
            new_parent.m_next_at_start_index = this_index;
        }
    }

    /**
     * @brief Returns the start index in the input text of this rule interval.
     *
     * @return The start index.
     */
    len_t start() const { return m_start; }

    /**
     * @brief Returns the rule id which this interval is an instance of.
     *
     * @return The rule id
     */
    len_t rule_id() const { return m_rule_id; }

    /**
     * @brief Checks whether this interval has a parent.
     *
     * @return true, if the parent's index is not @link INVALID, false otherwise
     */
    bool has_parent() const { return m_parent_index != INVALID; }

    /**
     * @brief
     *
     * @return
     */
    len_t parent_index() const { return m_parent_index; }

    len_t first_at_start_index() const { return m_first_at_start_index; }

    len_t next_at_start_index() const { return m_next_at_start_index; }

    void set_first_at_start_index(len_t interval) { m_first_at_start_index = interval; }

    void set_next_at_start_index(len_t interval) { m_next_at_start_index = interval; }

    /**
     * @brief Returns true, if the other interval is contained in the bounds of this one
     *
     * @return true If the other interval is contained in this one
     * @return false If the other interval only intersects or lies outside this one
     */
    static bool contains(len_t this_start, len_t this_len, len_t other_start, len_t other_len) {
        return this_start <= other_start && other_start + other_len <= this_start + this_len;
    }

    /**
     * @brief Checks, if this rule interval contains another.
     *
     * @tparam sampling The sampling parameter of the rule interval index data structure
     * @param other The other interval to check
     * @param rii The rule interval index data structure to look up data from. Rule intervals do not save their length
     * so it must be retrieved from a rule interval index data structure.
     *
     * @return true, if the other interval's borders are within (or equal to) this interval's borders. false otherwise
     */
    template<uint8_t sampling>
    bool contains(const RuleInterval &other, const RuleIntervalIndex<sampling> &rii) const {
        len_t self_length  = rii.rule_length(this->rule_id());
        len_t other_length = rii.rule_length(other.rule_id());
        return RuleInterval::contains(this->start(), self_length, other.start(), other_length);
    }

    /**
     * @brief Implement equality checks between rule intervals.
     *
     * This only checks its start position and its rule id. This should suffice to uniquely identify a rule interval.
     * There shouldn't be multiple rule intervals starting at the same index
     *
     * @param rhs The right hand side of the equality operator
     *
     * @return true, if the intervals are equal, false otherwise
     */
    bool operator==(const RuleInterval &rhs) const {
        return this->start() == rhs.start() && this->rule_id() == rhs.rule_id();
    }
};

static std::ostream &operator<<(std::ostream &os, RuleInterval const &i) __attribute__((unused));
static std::ostream &operator<<(std::ostream &os, RuleInterval const &i) {
    return os << "{ id: " << i.rule_id() << ", start: " << i.start()
              << ", parent: " << (i.has_parent() ? std::to_string(i.parent_index()) : "/")
              << ", first: " << i.first_at_start_index()
              << ", next: " << (i.next_at_start_index() != INVALID ? std::to_string(i.next_at_start_index()) : "/")
              << " }";
}

template<uint8_t sampling>
class RuleIntervalIndex {

    using Bucket = tdc::pred::dynamic::bucket_bv<len_t, sampling>;

    /**
     * @brief The length of the underlying string
     */
    size_t m_len;

    /**
     * @brief A predecessor data structure which contains the position at which a rule interval
     * starts
     */
    tdc::pred::dynamic::DynIndex<len_t, sampling, Bucket> m_pred;

    /**
     * @brief A vector which for each index in the input sequence contains
     * the index in @link m_rule_at_index of the most deeply nested rule interval that starts at
     * this index. If there is no such rule interval, the respective index contains the valie @link
     * INVALID
     */
    std::vector<len_t> m_rule_at_index;

    /**
     * @brief A vector of intervals, which contains all rule intervals in the order in which they
     * were inserted into the data structure.
     *
     * Since when a rule in the grammar is created, all occurences of said rule are replaced,
     * this vector has the property, that all rule intervals belonging to a certain rule,
     * occur contiguously.
     */
    std::vector<RuleInterval> m_interval_list;

    /**
     * @brief Contains a mapping from rule id, to the length of the fully expanded right side of the
     * rule.
     *
     * Here the index represents the rule id.
     */
    std::vector<len_t> m_rule_lengths;

    /**
     * @brief Inserts a rule interval into @link m_interval_list *only*.
     *
     * This method does not modify @link m_rule_lengths or @link m_rule_at_index, since it is not
     * always necessary to modify them upon inserting an interval. These must be modified manually,
     * should it be necesarry.
     *
     * @param interval The interval to be inserted.
     * @returns The index @link m_interval_list at which the interval was inserted.
     */
    len_t insert(RuleInterval &&interval) {
        m_interval_list.push_back(interval);
        return m_interval_list.size() - 1;
    }

  public:
    /**
     * @brief Creates a new index for a text of the given size.
     *
     * @param len The size of the input text.
     */
    RuleIntervalIndex(size_t len) : m_len{len}, m_pred{}, m_rule_at_index{}, m_interval_list{}, m_rule_lengths{} {
        // At the start there are no rule intervals at any point in the text, so all positions are invalid
        m_rule_at_index = std::vector(static_cast<unsigned int>(len), INVALID);
        // Create a new rule interval for the start rule and insert its data into the data structure
        RuleInterval interval(0, 0);
        interval.set_first_at_start_index(interval.start());
        m_interval_list.push_back(interval);
        m_rule_at_index[0] = 0;
        m_pred.insert(0);
        m_rule_lengths.push_back(len);
    }

    /**
     * @brief Checks whether a rule interval starts at the given index in the input text.
     *
     * @param index The index in the input text to check.
     *
     * @return true, if a rule interval starts at the index, false otherwise
     */
    bool has_interval_at(len_t index) { return m_rule_at_index[index] != INVALID; }

    /**
     * Gets the index in @link m_interval_list of the most deeply nested interval that starts at the given index in the
     * text
     *
     * @param index The text index to search for
     *
     * @return The index in @link m_interval_list of the interval if it exists, @link INVALID otherwise.
     */
    len_t deepest_interval_index_at(len_t index) const { return m_rule_at_index[index]; }

    /**
     * @brief Determines the index in @link m_interval_list of the rightmost interval with a start index less than or
     * equal to the given index.
     *
     * @param index The index in the input text to check.
     *
     * @return The index of the interval in @link m_interval_list
     */
    len_t floor_interval_index(int index) const {
        const len_t pred_index = m_pred.predecessor(index).key;
        return deepest_interval_index_at(pred_index);
    }

    /**
     * @brief Gets the actual interval corresponding to the id given.
     *
     * Note, that this is *not* (necessarily) an interval that starts at the given index in the
     * text. Rather, this returns the nth interval that was inserted into the data structure.
     * @param n The index into the @link m_interval_list vector.
     * @return The rule interval that corresponds to the given index.
     */
    RuleInterval &get(len_t n) { return m_interval_list[n]; }

    /**
     * @brief Gets the actual interval corresponding to the id given as a const reference.
     *
     * Note, that this is *not* (necessarily) an interval that starts at the given index in the
     * text. Rather, this returns the nth interval that was inserted into the data structure.
     * @param n The index into the @link m_interval_list vector.
     * @return The rule interval that corresponds to the given index.
     */
    const RuleInterval &get_const(len_t n) const { return m_interval_list[n]; }

    /**
     * Get the index in @link m_interval_list of the deepest nested interval that contains the given bounds
     * @param from The inclusive start index of the interval to check for
     * @param to The inclusive end index of the interval to check for
     * @return Return index in @link m_interval_list of the deepest nested interval that contains the interval [from,
     * to] if there is such an interval. @link INVALID otherwise
     */
    len_t interval_index_containing(size_t from, size_t to) const {
        // We start with the most deeply nested interval that could contain our bounds
        auto current_index = floor_interval_index(from);

        // If we find that the index is invalid, there is no interval that contains from and to. This should never
        // happen, since the start rule should always contain any interval.
        while (current_index != INVALID) {
            // We get the interval corresponding to the index. If it contains our bounds, then we have found the
            // interval we are looking for.
            auto current = &get_const(current_index);
            if (current->start() <= from && to <= end(*current)) {
                break;
            }

            // If not, we look at the least deeply nested interval at the same start index.
            auto first_at_index = get_const(current->first_at_start_index());
            // If this interval contains current, that means that the interval we are looking for is contained in the
            // "chain" between our current interval and the least deeply nested interval at the same start index.
            if (first_at_index.start() <= from && to <= end(first_at_index)) {
                current_index = current->parent_index();
            } else {
                // If even the least deeply nested interval at this start index does not contain our bounds, then none
                // at this start index do. We can skip all of them.
                current_index = current->first_at_start_index();
                current       = &get_const(current_index);
                // No use in checking the first at this start index either, since we already determined that it does not
                // contain our bounds. We jump to its parent if it exists.
                if (current->has_parent()) {
                    current_index = current->parent_index();
                }
            }
        }
        return current_index;
    }

    /**
     * Get the index in @link m_interval_list of the deepest nested interval that contains the given index
     * @param index The index in the text at which to search for
     * @return Return the deepest nested interval that contains the index if there is
     * such an interval. null otherwise
     */
    len_t interval_index_containing(size_t index) const { return interval_index_containing(index, index); }

    /**
     * @brief Retrurns the length of the rule with the given id.
     *
     * @param rule_id The rule id
     *
     * @return The length of the rule with the given rule id
     */
    len_t rule_length(len_t rule_id) const { return m_rule_lengths[rule_id]; }

    // Convenience Methods:

    /**
     * @brief Returns the parent of the given rule interval.
     *
     * @param interval The interval whose parent to return.
     *
     * @return The interval's parent interval.
     */
    RuleInterval &parent(const RuleInterval &interval) { return get(interval.parent_index()); }

    /**
     * @brief Returns the least deeply nested interval that starts at the same index as the given interval.
     *
     * @param interval The interval
     *
     * @return The least deeply nested interval that starts at the same index as the given interval.
     */
    RuleInterval &first_at_start_index(const RuleInterval &interval) { return get(interval.first_at_start_index()); }

    /**
     * @brief Returns the next more deeply nested interval that starts at the same start index.
     *
     * @param interval The interval
     *
     * @return The next more deeply nested interval that starts at the same start index.
     */
    RuleInterval &next_at_start_index(const RuleInterval &interval) { return get(interval.next_at_start_index()); }

    /**
     * @brief Determines the rightmost interval with a start index less than or
     * equal to the given index.
     *
     * @param index The index in the input text to check.
     *
     * @return The predecessor interval of the given index.
     */
    RuleInterval &floor_interval(len_t index) { return get(floor_interval_index(index)); }

    /**
     * Returns the most deeply nested interval that starts at this index
     *
     * @param index The index to search for
     *
     * @return The interval if such exists
     */
    RuleInterval &deepest_interval_at(len_t index) { return get(deepest_interval_index_at(index)); }

    /**
     * @brief Return the most deeply nested interval that contains the given bounds.
     *
     * @param from The inclusive lower bound.
     * @param to The inclusive upper bound.
     *
     * @return The most deeply nested interval that contains the given bounds.
     */
    RuleInterval &interval_containing(len_t from, len_t to) { return get(interval_index_containing(from, to)); }

    /**
     * @brief Return the most deeply nested interval that contains the given index in the text.
     *
     * @param i The index in the text, which the returned interval should contain.
     *
     * @return The most deeply nested interval that contains the given index.
     */
    RuleInterval &interval_containing(len_t i) { return get(interval_index_containing(i)); }

    /**
     * @brief Returns the interval's (inclusive) end index in the text.
     *
     * @param interval The interval whose end index to return.
     *
     * @return
     */
    len_t end(const RuleInterval &interval) const { return interval.start() + rule_length(interval.rule_id()) - 1; }

    /**
     * @brief Marks the area with the given rule id.
     *
     * In essence, this inserts a new rule interval with the given data into the data structure,
     * marking the substitution of a subsequence in the input.
     *
     * @param rule_id The rule id
     * @param start The inclusive start index in the input text.
     * @param end The inclusive end index in the input text.
     */
    void mark_new(len_t rule_id, len_t start, len_t end) {
        len_t index;
        // Create a new interval and insert into the m_interval_list vector.
        // The interval does not necessarily need to be inserted into the other data structures, so
        // this is done later if necessary
        // We also retrieve the index of the interval in
        {
            RuleInterval interval(rule_id, start);
            index = insert(std::move(interval));
        }

        // The interval object is retrieved from the vector
        RuleInterval &interval = get(index);
        if (m_rule_lengths.size() == rule_id) {
            m_rule_lengths.push_back(end - start + 1);
        }

        if (!has_interval_at(start)) {
            // If there is no other interval that starts at this index, all we need to do is determine the parent
            // interval, link them accordingly and add them into m_rule_at_index and the predecessor data structure.
            len_t parent_index = interval_index_containing(start, end);
            interval.insert_parent(index, parent_index, m_interval_list);
            interval.set_first_at_start_index(index);
            m_rule_at_index[start] = index;
            m_pred.insert(start);
        } else {
            // If there is an interval that start at this index, determine its index in m_interval_list
            // and retrieve the interval
            len_t         current_index = deepest_interval_index_at(start);
            RuleInterval *current       = &get(current_index);
            if (current->contains(interval, *this)) {
                // If the new interval fits into the deepest interval at this start index, then it must be the new
                // deepest interval In this case, the previously deepest interval becomes the new interval's parent
                interval.insert_parent(index, current_index, m_interval_list);
                // The first (= least deeply nested) interval for our new interval at this start index is obviously the
                // same as its parent's
                interval.set_first_at_start_index(current->first_at_start_index());
                m_rule_at_index[start] = index;
                m_pred.insert(start);
            } else {
                // If there is an interval at this index already and the new interval does not fit into the deepest new
                // interval at this index, we have to do some more work.
                // Get the least deeply nested interval at this index.
                auto &first_at_index = first_at_start_index(*current);
                // If the least deeply nested (and therefore largest) interval at this index fits in our new interval,
                // then our new interval is the new least deeply nested one
                const bool is_new_first = interval.contains(first_at_index, *this);

                // We search from the bottom up for the place in the interval "stack" that our new interval fits into
                while (true) {
                    // If our new interval is the least deeply nested, then all children that start at this index must
                    // have their values updated to our new interval
                    if (is_new_first) {
                        current->set_first_at_start_index(index);
                    }
                    // Get our current interval's parent. Note that current starts as the most deeply nested interval
                    auto &par = parent(*current);
                    // We search for the first interval which our new interval fits into. If the current interval has no
                    // parent, or the parent starts at a different index, we stop searching of course. We can arrive at
                    // this point if the new interval is the least deeply nested interval at this index.
                    if (!par.has_parent() || par.start() != start || par.contains(interval, *this)) {
                        break;
                    }
                    current_index = current->parent_index();
                    current       = &par;
                }
                // We insert the new interval into the "stack" of intervals
                auto &par = parent(*current);
                current->insert_parent(current_index, index, m_interval_list);
                // We inserted the new interval above current and need to set its first_at_start_index value. If our new
                // interval has a parent (if so, it would have been set during insertion), check if current's former
                // parent starts at the same index as our new interval. If so, we can just take its first_at_start_index
                // value.
                // Otherwise, our new interval is the least deepyl nested and therefore first at this index
                interval.set_first_at_start_index(
                    interval.has_parent() && interval.start() == par.start() ? par.first_at_start_index() : index);
            }
        }

        // When inserting the new interval it could be that the new interval is inserted "between" intervals at other
        // indices and their parent interval.
        // So we need to update the parent pointers of the intervals where this happened.
        // We check every least deeply nested interval that is contained in the bounds of the new interval and set their
        // parent to the new interval if its parent is equal to the new interval's parent. We check this condition,
        // because if so, that means that the new interval was inserted between the other interval and its parent.
        for (RuleInterval *current = &floor_interval(end); current->start() > start;
             current               = &floor_interval(current->start() - 1)) {
            auto &first = get(current->first_at_start_index());
            if (interval.contains(first, *this) && first.parent_index() == interval.parent_index()) {
                first.insert_parent(current->first_at_start_index(), index, m_interval_list);
            }
        }
    }

    /**
     * @brief Returns the length of the underlying text.
     *
     * @return The length of the underlying text.
     */
    size_t text_len() const { return m_len; }
};

} // namespace tdc::grammar::areacomp
