#pragma once

#include <cstdio>
#include <tdc/pred/dynamic/dynamic_index.hpp>
#include <tdc/pred/dynamic/buckets/buckets.hpp>

namespace tdc::grammar::areacomp {
    class RuleInterval : public std::enable_shared_from_this<RuleInterval> {
        const size_t m_rule_id;
        const size_t m_start;
        const size_t m_end;

        std::shared_ptr<RuleInterval> m_parent;
        std::shared_ptr<RuleInterval> m_first_at_start_index;
        std::shared_ptr<RuleInterval> m_next_at_start_index;

    public:
        RuleInterval(size_t rule_id, size_t start, size_t end) : m_rule_id{rule_id}, m_start{start}, m_end{end} {}

        void insert_parent(std::shared_ptr<RuleInterval> new_parent) {
            if (has_parent()) {
                new_parent->m_parent = this->m_parent;
                if (this->m_parent->m_start == new_parent->m_start) {
                    new_parent->m_first_at_start_index = this->m_parent->m_first_at_start_index;
                }
            }
            this->m_parent = new_parent;
            if (new_parent->m_start == this->m_start) {
                new_parent->m_next_at_start_index = this->shared_from_this();
            }
        }

        size_t start() const {
            return m_start;
        }

        size_t end() const {
            return m_end;
        }

        size_t rule_id() const {
            return m_rule_id;
        }

        bool has_parent() const {
            return m_parent != nullptr;
        }

        std::shared_ptr<RuleInterval> parent() {
            return m_parent;
        }

        std::shared_ptr<RuleInterval> first_at_start_index() {
            return m_first_at_start_index;
        }

        std::shared_ptr<RuleInterval> next_at_start_index() {
            return m_next_at_start_index;
        }

        void set_first_at_start_index(std::shared_ptr<RuleInterval> interval) {
            m_first_at_start_index = interval;
        }

        void set_next_at_start_index(std::shared_ptr<RuleInterval> interval) {
            m_next_at_start_index = interval;
        }

        /**
         * @brief Returns true, if the other interval is contained in the bounds of this one
         *
         * @param other The other interval
         * @return true If the other interval is contained in this one
         * @return false If the other interval only intersects or lies outside this one
         */
        bool contains(RuleInterval &other) {
            return this->m_start <= other.m_start && other.m_end <= this->m_end;
        }


    };

    template<uint8_t sampling=16>
    class RuleIntervalIndex {

        using Bucket = tdc::pred::dynamic::bucket_bv<size_t, sampling>;

    private:
        size_t m_top_level_rule_id;
        size_t m_len;
        tdc::pred::dynamic::DynIndex<size_t, sampling, Bucket> m_pred;
        std::unordered_map<size_t, std::shared_ptr<RuleInterval>> m_interval_map;

        void insert(size_t pos, std::shared_ptr<RuleInterval> interval) {
            m_interval_map[pos] = std::move(interval);
            m_pred.insert(pos);
            
        }

        void insert(size_t pos, RuleInterval &&interval) {
            insert(pos, std::shared_ptr<RuleInterval>(interval));
        }

    public:
        RuleIntervalIndex(size_t top_level_rule_id, size_t len) :
                m_top_level_rule_id{top_level_rule_id},
                m_len{len},
                m_pred{},
                m_interval_map{} {
            auto interval = std::make_shared<RuleInterval>(m_top_level_rule_id, 0, len);
            interval->set_first_at_start_index(interval);

            insert(0, interval);
        }

        /**
         * Gets the most deeply nested interval that starts at this index
         * @param index The index to search for
         * @return The interval if such exists, null otherwise
         */
        std::shared_ptr<RuleInterval> interval_at_start_index(size_t index) {
            return m_interval_map[index];
        }

        std::shared_ptr<RuleInterval> floor_interval(int index) {
            const size_t pred_index = m_pred.predecessor(index).key;
            return m_interval_map[pred_index];
        }

        /**
         * Get the deepest nested interval that contains the given interval
         * @param from The inclusive start index of the interval to check for
         * @param to The inclusive end index of the interval to check for
         * @return Return the deepest nested interval that contains the interval [from, to] if there is such an interval. null otherwise
         */
        std::shared_ptr<RuleInterval> interval_containing(size_t from, size_t to) {
            auto current = floor_interval(from);

            while (current != nullptr) {
                if (current->start() <= from && to <= current->end()) {
                    return current;
                }

                if (current->first_at_start_index()->start() <= from && to <= current->first_at_start_index()->end()) {
                    current = current->parent();
                } else {
                    current = current->first_at_start_index();
                    if (current->has_parent()) {
                        current = current->parent();
                    }
                }
            }
            return nullptr;
        }

        std::shared_ptr<RuleInterval> interval_containing(size_t index) {
            return interval_containing(index, index);
        }

        /**
         * Marks the area with the given rule id
         * @param ruleId The rule id
         * @param start inclusive
         * @param end inclusive
         */
        void mark(size_t rule_id, size_t start, size_t end) {
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
                    while(true) {
                        if (is_new_first) {
                            current->set_first_at_start_index(interval);
                        }
                        // If there are no more less-deeply nested intervals at this start index, break
                        if (!current->has_parent() || current->parent()-> start() != start || current->parent()->contains(*interval)) {
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
        }

        size_t text_len() const {
            return m_len;
        }


    };
}
