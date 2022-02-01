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

template<uint8_t sampling = 16>
class RuleIntervalIndex;

class RuleInterval {

    const len_t m_rule_id;
    const len_t m_start;
    //const size_t m_end;

    len_t m_parent_index;
    len_t m_first_at_start_index;
    len_t m_next_at_start_index;

    //std::shared_ptr<RuleInterval> m_parent;
    //std::shared_ptr<RuleInterval> m_first_at_start_index;
    //std::shared_ptr<RuleInterval> m_next_at_start_index;

  public:
    RuleInterval(len_t rule_id, len_t start) :
        m_rule_id{rule_id},
        m_start{start},
        m_parent_index{INVALID},
        m_first_at_start_index{INVALID},
        m_next_at_start_index{INVALID} {}

    void insert_parent(len_t this_index, len_t new_parent_index, std::vector<RuleInterval> &interval_list) {
        auto &new_parent = interval_list[new_parent_index];
        if (has_parent()) {
            auto &parent = interval_list[m_parent_index];

            new_parent.m_parent_index = this->m_parent_index;
            if (parent.m_start == new_parent.m_start) {
                new_parent.m_first_at_start_index = parent.m_first_at_start_index;
            }
        }
        this->m_parent_index = new_parent_index;
        if (new_parent.m_start == this->m_start) {
            new_parent.m_next_at_start_index = this_index;
        }
    }

    len_t start() const {
        return m_start;
    }

    len_t rule_id() const {
        return m_rule_id;
    }

    bool has_parent() const {
        return m_parent_index != INVALID;
    }

    len_t parent_index() const {
        return m_parent_index;
    }

    len_t first_at_start_index() const {
        return m_first_at_start_index;
    }

    len_t next_at_start_index() const {
        return m_next_at_start_index;
    }

    void set_first_at_start_index(len_t interval) {
        m_first_at_start_index = interval;
    }

    void set_next_at_start_index(len_t interval) {
        m_next_at_start_index = interval;
    }

    /**
         * @brief Returns true, if the other interval is contained in the bounds of this one
         *
         * @return true If the other interval is contained in this one
         * @return false If the other interval only intersects or lies outside this one
         */
    static bool contains(len_t this_start, len_t this_len, len_t other_start, len_t other_len) {
        return this_start <= other_start && other_start + other_len <= this_start + this_len;
    }

    template<uint8_t sampling>
    static bool contains(const RuleInterval &self, const RuleInterval &other, RuleIntervalIndex<sampling> &rii);

    bool operator==(const RuleInterval &rhs) const {
        // This should suffice to identify a rule interval. There shouldn't be multiple rule intervals starting at the same index
        return this->start() == rhs.start() && this->rule_id() == rhs.rule_id();
    }
};

static std::ostream &operator<<(std::ostream &os, RuleInterval const &i) {
    return os << "{ id: " << i.rule_id() << ", start: " << i.start() << ", parent: " << (i.has_parent() ? std::to_string(i.parent_index()) : "/") << ", first: " << i.first_at_start_index() << ", next: " << (i.next_at_start_index() != INVALID ? std::to_string(i.next_at_start_index()) : "/")
              << " }";
}

template<uint8_t sampling>
bool RuleInterval::contains(const RuleInterval &self, const RuleInterval &other, RuleIntervalIndex<sampling> &rii) {
    len_t self_start   = self.start();
    len_t self_length  = rii.rule_length(self.rule_id());
    len_t other_start  = other.start();
    len_t other_length = rii.rule_length(other.rule_id());
    return RuleInterval::contains(self_start, self_length, other_start, other_length);
}

template<uint8_t sampling>
class RuleIntervalIndex {

    using Bucket = tdc::pred::dynamic::bucket_bv<len_t, sampling>;

    size_t                                                m_top_level_rule_id;
    size_t                                                m_len;
    tdc::pred::dynamic::DynIndex<len_t, sampling, Bucket> m_pred;
    std::vector<len_t>                                    m_rule_at_index;
    std::vector<RuleInterval>                             m_interval_list;
    std::vector<len_t>                                    m_rule_lengths;

    len_t insert(RuleInterval &&interval) {
        m_interval_list.push_back(interval);
        return m_interval_list.size() - 1;
    }

  public:
    RuleIntervalIndex(size_t top_level_rule_id, size_t len) :
        m_top_level_rule_id{top_level_rule_id},
        m_len{len},
        m_pred{},
        m_rule_at_index{},
        m_interval_list{},
        m_rule_lengths{} {

        m_rule_at_index = std::vector(static_cast<unsigned int>(len), INVALID);
        RuleInterval interval(m_top_level_rule_id, 0);
        interval.set_first_at_start_index(interval.start());
        m_interval_list.push_back(interval);
        //std::fill(m_rule_at_index.begin(), m_rule_at_index.end(), INVALID);
        m_rule_at_index[0] = 0;
        m_pred.insert(0);
        m_rule_lengths.push_back(len);
    }

    bool has_interval_at(len_t index) {
        return m_rule_at_index[index] != INVALID;
    }

    /**
         * Gets the index of the most deeply nested interval that starts at this index
         * @param index The index to search for
         * @return The interval if such exists, null otherwise
         */
    len_t deepest_interval_index_at(len_t index) {
        return m_rule_at_index[index];
    }

    len_t floor_interval_index(int index) {
        const len_t pred_index = m_pred.predecessor(index).key;
        return deepest_interval_index_at(pred_index);
    }

    /**
      * @brief Gets the actual interval corresponding to the id given.
      *
      * Note, that this is *not* (necessarily) an interval that starts at the given index in the text.
      * Rather, this returns the nth interval that was inserted into the data structure. 
      * @param n The index into the @link m_interval_list vector.
      * @return The rule interval that corresponds to the given index.
    */
    RuleInterval &get(len_t n) {
        return m_interval_list[n];
    }

    /**
         * Get the deepest nested interval that contains the given interval
         * @param from The inclusive start index of the interval to check for
         * @param to The inclusive end index of the interval to check for
         * @return Return the deepest nested interval that contains the interval [from, to] if there is such an interval. null otherwise
         */
    len_t interval_index_containing(size_t from, size_t to) {
        auto current_index = floor_interval_index(from);

        while (current_index != INVALID) {
            auto current = &get(current_index);
            if (current->start() <= from && to <= end(*current)) {
                break;
            }

            auto first_at_index = first_at_start_index(*current);
            if (first_at_index.start() <= from && to <= end(first_at_index)) {
                current_index = current->parent_index();
            } else {
                current_index = current->first_at_start_index();
                current       = &get(current_index);
                if (current->has_parent()) {
                    current_index = current->parent_index();
                }
            }
        }
        return current_index;
    }

    len_t interval_index_containing(size_t index) {
        return interval_index_containing(index, index);
    }

    len_t rule_length(len_t rule_id) {
        return m_rule_lengths[rule_id];
    }

    // Convenience Methods:

    RuleInterval &parent(RuleInterval &interval) {
        return get(interval.parent_index());
    }

    RuleInterval &first_at_start_index(RuleInterval &interval) {
        return get(interval.first_at_start_index());
    }

    RuleInterval &next_at_start_index(RuleInterval &interval) {
        return get(interval.next_at_start_index());
    }

    RuleInterval &floor_interval(len_t index) {
        return get(floor_interval_index(index));
    }

    RuleInterval &deepest_interval_at(len_t index) {
        return get(deepest_interval_index_at(index));
    }

    RuleInterval &interval_containing(len_t from, len_t to) {
        return get(interval_index_containing(from, to));
    }
    RuleInterval &interval_containing(len_t i) {
        return get(interval_index_containing(i));
    }

    len_t end(RuleInterval &interval) {
        return interval.start() + rule_length(interval.rule_id()) - 1;
    }

    /**
         * Marks the area with the given rule id
         * @param ruleId The rule id
         * @param start inclusive
         * @param end inclusive
         */
    /*void mark(size_t rule_id, size_t start, size_t end) {
        auto interval = std::make_shared<RuleInterval>(rule_id, start, end);
        {
            auto current = interval_at_start_index(start);

            if (current == nullptr) {
                auto parent = interval_containing(start, end);
                interval->insert_parent(parent);
                interval->set_first_at_start_index(interval);
                insert(start, interval);
                // If this interval is the new deepest nested interval
            } else if (current->contains(*interval)) {
                interval->insert_parent(current);
                interval->set_first_at_start_index(current->first_at_start_index());
                // since current is now the deepest interval, it replaces the previous deepest interval
                insert(start, interval);
            } else {
                // Whether the interval will be the least deeply nested at this start index
                const bool is_new_first = interval->contains(*(current->first_at_start_index()));
                while (true) {
                    if (is_new_first) {
                        current->set_first_at_start_index(interval);
                    }
                    // If there are no more less-deeply nested intervals at this start index, break
                    if (!current->has_parent() || current->parent()->start() != start || current->parent()->contains(*interval)) {
                        break;
                    }
                    current = current->parent();
                }
                current->insert_parent(interval);
                interval->set_first_at_start_index(interval->has_parent() && interval->start() == interval->parent()->start() ? interval->parent()->first_at_start_index() : interval);
            }
        }

        for (auto current = floor_interval(end); current->start() > start; current = floor_interval(current->start() - 1)) {

            auto first = current->first_at_start_index();
            if (interval->contains(*first) && first->parent() == interval->parent()) {
                first->insert_parent(interval);
            }
        }
    }*/

    void mark_new(len_t rule_id, len_t start, len_t end) {
        len_t index;
        {
            RuleInterval interval(rule_id, start);
            index = insert(std::move(interval));
        }

        RuleInterval &interval = get(index);
        if (m_rule_lengths.size() == rule_id) {
            m_rule_lengths.push_back(end - start + 1);
        }

        {
            if (!has_interval_at(start)) {
                len_t parent_index = interval_index_containing(start, end);
                interval.insert_parent(index, parent_index, m_interval_list);
                interval.set_first_at_start_index(index);
                m_rule_at_index[start] = index;
                m_pred.insert(start);
            } else {
                len_t         current_index = deepest_interval_index_at(start);
                RuleInterval *current       = &get(current_index);
                if (RuleInterval::contains(current->start(), rule_length(current->rule_id()), start, rule_length(rule_id))) {
                    interval.insert_parent(index, current_index, m_interval_list);
                    interval.set_first_at_start_index(current->first_at_start_index());
                    m_rule_at_index[start] = index;
                    m_pred.insert(start);
                } else {
                    auto      &first_at_index = first_at_start_index(*current);
                    const bool is_new_first   = RuleInterval::contains(start, end - start + 1, first_at_index.start(), rule_length(first_at_index.rule_id()));
                    while (true) {
                        if (is_new_first) {
                            current->set_first_at_start_index(index);
                        }
                        auto &par = parent(*current);
                        if (!current->has_parent() || par.start() != start || RuleInterval::contains(par.start(), rule_length(par.rule_id()), start, end - start + 1)) {
                            break;
                        }
                        current_index = current->parent_index();
                        current       = &par;
                    }

                    auto &par = parent(*current);
                    current->insert_parent(current_index, index, m_interval_list);
                    interval.set_first_at_start_index(interval.has_parent() && interval.start() == par.start() ? par.first_at_start_index() : index);
                }
            }
        }

        for (RuleInterval *current = &floor_interval(end); current->start() > start; current = &floor_interval(current->start() - 1)) {
            auto &first = get(current->first_at_start_index());
            if (RuleInterval::contains(start, end - start + 1, first.start(), rule_length(first.rule_id())) && first.parent_index() == interval.parent_index()) {
                first.insert_parent(current->first_at_start_index(), index, m_interval_list);
            }
        }
    }

    size_t text_len() const {
        return m_len;
    }
};

} // namespace tdc::grammar::areacomp
