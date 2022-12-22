#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <queue>
#include <ranges>
#include <string>
#include <tudocomp/compressors/areacomp/ChildArray.hpp>
#include <tudocomp/compressors/areacomp/Consts.hpp>
#include <tudocomp/compressors/areacomp/RuleIntervalIndex.hpp>
#include <tudocomp/compressors/areacomp/areafunctions/AreaFunction.hpp>
#include <tudocomp/ds/DSManager.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/grammar/Grammar.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp_stat/StatPhase.hpp>
#include <unordered_set>
#include <vector>

namespace tdc::grammar::areacomp {

/**
 * @brief The class handling the factorization of rules in areacomp
 *
 * This class contains the necessary datastuctures for compressing an input with Areacomp.
 */
template<size_t sampling>
class Ruleset {

    /**
     * @brief The underlying view into the text
     */
    io::InputView &m_underlying;

    /**
     * @brief The rule interval index data structure which keeps track of substituted substrings.
     */
    RuleIntervalIndex<sampling> m_interval_index;

    /**
     * @brief The number of rules in the grammar. This is used to get the next free rule id.
     */
    size_t m_num_rules;

    /**
     * @brief The minimum area value an interval has to reach to be considered.
     */
    size_t m_area_threshold;

  public:
    /**
     * @brief Construct a new ruleset for the given input view.
     *
     * This constructor does not automatically compress.
     * It creates a single rule mapping to the entire input text.
     */
    Ruleset(io::InputView &underlying, size_t area_threshold = 0) :
        m_underlying{underlying},
        m_interval_index{underlying.size()},
        m_num_rules{1},
        m_area_threshold{area_threshold} {}

  private:
    /**
     * @brief Returns the next unused rule id
     */
    size_t next_rule_id() { return m_num_rules++; }

    /**
     * Checks whether the given interval can be substituted without violating other previous substitutions in this area.
     * In essence, this checks whether there is an already substituted interval in which a start symbol and and end
     * symbol can be found, which correspond to the indices from and to in the input String.
     *
     * For example. Take the following grammar for the string "abacaba":
     *
     * S -> AcA
     * A -> aba
     *
     * This method returns true for the input [0, 3]
     * This is because the 0 index here corresponds to either the first "A" in Rule S or the first "a" in Rule A and the
     * 3 index corresponds to the "c", both in Rule S. Here, a start- and end symbol can be found that are in the right
     * side of the same rule and correspond to the indices. These being the "A" and "c" in Rule S respectively.
     *
     * This method returns false for the input [1, 3], since the only symbol that corresponds to the index 1 is the "b"
     * in rule A, and for index 3 it is only the "c". So there is no way to find a start- and end symbol which both lie
     * in the same substituted interval.
     *
     * This method also returns false for the input [0, 5]. Index 0 corresponds to the first "A" in rule S and the first
     * "a" in rule A, while index 5 corresponds only to the "b" in rule A. While start- and end symbols can be found,
     * which are of the same rule ("a" and "b" of rule A), they are *not* of the same substituted interval. While the
     * "a" is of the first substituted instance of rule A, the "b" is of the second substituted instance. As a result
     * this substitution would also be invalid.
     *
     * @param from The lower bound of the range (inclusive)
     * @param to The upper bound of the range (inclusive)
     * @return true, if the interval starts in the same rule range as it started. false otherwise
     */
    bool substitution_allowed(size_t from, size_t to) {
        RuleInterval *from_interval = &m_interval_index.interval_containing(from);

        // If the start index is the start of this interval, it might imply that this is a non-terminal in a less-deeply
        // nested rule. In that case, we iterate upwards by using the parent pointers in order to find the first
        // interval, which contains the end index also. Mind, that only intervals that start at the same index, and the
        // parent of the last such interval can be considered here. This is because while the start index is equal to
        // the current interval's start position, that means that the start index describes the position of a
        // non-terminal in its parent interval.
        while (from == from_interval->start() && to > m_interval_index.end(*from_interval) &&
               from_interval->has_parent()) {
            from_interval = &m_interval_index.parent(*from_interval);
        }
        // If there is no such interval, that would also contain "to", the substitution can't be allowed
        if (to > m_interval_index.end(*from_interval)) {
            return false;
        }

        RuleInterval *to_interval = &m_interval_index.interval_containing(to);
        while (to_interval != from_interval && to == m_interval_index.end(*to_interval) && to_interval->has_parent()) {
            to_interval = &m_interval_index.parent(*to_interval);
        }
        return *from_interval == *to_interval;
    }

    /**
     * Determines, whether there are multiple occurrences of the same pattern in distinct contexts regarding already
     * existing rules For example, the string "aaaaaa" and the grammar
     *
     * R0 -> R1 R1
     * R1 -> aaa
     *
     * The algorithm might try to replace the pattern "aa" at positions 0, 2 and 4, since it only computed the Suffix-
     * and LCP-Array in the beginning. Since in the algorithm, pattern occurrences that cross the boundaries of
     * pre-existing rules are removed before this algorithm is called, the occurrence at position 2 is removed. This is
     * the case, because it would cross the boundary from the first R1 range into the second R1 range, if you look at
     * the right side of R0. But even so, factoring out "aa" at index 0 and 4 is not possible, since the pattern would
     * appear once as the first "aa" in R1 and once as the second "aa" in R1. This method detects, if the given
     * positions cannot be factored out.
     *
     * @param positions The positions to check
     * @return true, if there are positions that can be factored out, false otherwise
     */
    bool differing_occurrences(std::vector<size_t> &positions, len_t pattern_length) {
        if (positions.empty()) {
            return false;
        }

        size_t first_rule_id = INVALID;
        size_t first_offset  = INVALID;

        for (auto pos : positions) {
            if (pos == INVALID) {
                continue;
            }

            // We determine the rule in which the pattern appears in the grammar, the corresponding id, and the offset
            // of the pattern in the rule's right side
            RuleInterval &rule_interval = m_interval_index.interval_containing(pos, pos + pattern_length - 1);
            len_t         rule_id       = rule_interval.rule_id();
            len_t         offset        = pos - rule_interval.start();

            // We populate the values we find in the first occurrence
            if (first_rule_id == INVALID) {
                first_rule_id = rule_id;
                first_offset  = offset;
            } else if (rule_id != first_rule_id || offset != first_offset) {
                // If the rule id of the current interval is different, then we must have found two distinct occurrences
                // of this pattern. If on the other hand we find that the rule_id is the same as the one we have found
                // before, but the offset of the pattern's position inside the rule is different, then we also have
                // found a second distinct occurrence.
                return true;
            }
        }
        return false;
    }

    /**
     * Filters out the occurrences of the pattern with length len and at the given positions, which start in one rule
     * range and end in another. Also filters out overlapping occurrences.
     * @param positions The positions to filter
     * @param len The length of the pattern
     * @return The amount of valid positions
     *
     * @see #substitution_allowed(size_t, size_t)
     */
    size_t clean_positions(std::vector<size_t> &positions, size_t len) {
        size_t count    = 0;
        size_t previous = INVALID;
        for (unsigned long &pos : positions) {
            // If the current position does not overlap with the previous one and the substitution is allowed, keep the
            // position
            if ((previous == INVALID || previous + len <= pos) && substitution_allowed(pos, pos + len - 1)) {
                previous = pos;
                count++;
            } else {
                // otherwise invalidate it
                pos = INVALID;
            }
        }

        return count;
    }

    /**
     * Factorizes a repeated sequence. All occurrences of the pattern in the grammar will be replaced with a new rule
     * This method makes the following assumptions
     * All occurences are non overlapping.
     * The positions are sorted in ascending order.
     * @param len The length of the subsequence to replace in terminal characters
     * @param positions The start positions of the occurences in the original string
     */
    void substitute(std::vector<size_t> &positions, size_t len) {
        if (len <= 1 || positions.size() < 2)
            return;
        const size_t next_id = next_rule_id();

        // Iterate through the positions and substitute each occurrence
        for (auto position : positions) {
            if (position == INVALID) {
                continue;
            }

            m_interval_index.mark_new(next_id, position, position + len - 1);
        }
    }

  public:
    /**
     * Compresses the ruleset using an area function.
     * The area function determines how the intervals in the priority queue are prioritised.
     *
     * @param fun The area function used to prioritise intervals in the lcp array
     * @param text_ds The data structure provider
     */
    template<typename Area, typename TextDS>
    requires AreaFun<Area, TextDS>
    void compress(Area fun, TextDS text_ds) {

        const auto &sa  = text_ds.template get<ds::SUFFIX_ARRAY>();
        const auto &lcp = text_ds.template get<ds::LCP_ARRAY>();

        StatPhase global_phase("Build Child Array");

        areacomp::ChildArray<const DynamicIntVector> cld(lcp, m_underlying.size());

        global_phase.split("Get LCP Intervals");

        auto  intervals  = cld.template get_lcp_intervals_with_area<Area, TextDS>(fun, text_ds, m_area_threshold, 2);
        auto &start_list = intervals.first;
        auto &len_list   = intervals.second;

        global_phase.split("Calculate permutation");
        std::vector<len_t> order(start_list.size(), 0);
        std::iota(order.begin(), order.end(), 0);

        std::sort(order.begin(), order.end(), [&](auto a, auto b) {
            auto area_a = fun.area(text_ds, cld, start_list[a] + 1, start_list[a] + len_list[a]);
            auto area_b = fun.area(text_ds, cld, start_list[b] + 1, start_list[b] + len_list[b]);
            return area_a > area_b;
        });

        global_phase.split("Compression Loop");

        size_t index = 0;
        while (index < start_list.size()) {
            // StatPhase sub_phase("Get best area");
            //  Poll the best interval
            auto start = start_list[order[index]];
            auto end   = start + len_list[order[index]];
            // sub_phase.split("Get positions");

            // The positions at which the pattern can be found
            std::vector<size_t> positions;
            positions.reserve(end - start + 2);

            // Get the length of the longest common prefix in this range of the lcp array
            // This will be the length of the pattern that is to be replaced.
            size_t len = cld.l_value(start, end);

            for (size_t i = start; i <= end; ++i) {
                positions.push_back(sa[i]);
            }

            // This means there is no repeated subsequence of 2 or more characters. In this case, abort
            if (len <= 1) {
                index++;
                continue; // TODO put this back to break
            }

            // sub_phase.split("Sort positions");

            std::sort(positions.begin(), positions.end());

            // sub_phase.split("Clean positions");
            //  Remove positions at which substituting is impossible
            const size_t position_count = clean_positions(positions, len);
            if (position_count <= 1) {
                index++;
                continue;
            }

            // sub_phase.split("Differing Occurrences");
            //  Check, if the found occurrences are actually distinct in the current grammar
            const bool multiple_occurrences = differing_occurrences(positions, len);

            // If there is only one occurrence left, there is no use in creating a rule in the grammar
            if (!multiple_occurrences) {
                index++;
                continue;
            }

            // sub_phase.split("Substitute");

            substitute(positions, len);
            index++;
        }
    }

    /**
     * @brief Returns the rule interval index data structure
     *
     * @return The rule interval index data structure
     */
    RuleIntervalIndex<sampling> &interval_index() { return m_interval_index; }

    /**
     * @brief Returns a somewhat readable representation of the rule interval index.
     *
     * @return A string representing the data structure.
     */
    std::string interval_index_to_string() {
        std::vector<std::string> buf;

        len_t current_index = interval_index().floor_interval_index(interval_index().text_len() - 1);

        while (current_index != INVALID) {
            RuleInterval      current = interval_index().get(current_index);
            std::stringstream ss;
            ss << current.start() << ": [";
            len_t current_vertical_index = current.first_at_start_index();
            while (true) {
                RuleInterval current_vertical = interval_index().get(current_vertical_index);
                ss << current_vertical.rule_id() << ": " << m_interval_index.end(current);
                if (current_vertical.next_at_start_index() == INVALID) {
                    ss << "]";
                    buf.push_back(ss.str());
                    break;
                } else {
                    ss << ", ";
                    current_vertical_index = current_vertical.next_at_start_index();
                }
            }

            if (current.start() == 0) {
                break;
            }

            current_index = interval_index().floor_interval_index(current.start() - 1);
        }

        std::stringstream ss;

        std::reverse(buf.begin(), buf.end());
        for (const auto &str : buf) {
            ss << str << ", ";
        }

        return ss.str();
    }

    /**
     * @brief Converts the ruleset into a tudocomp grammar.
     *
     * @return The grammar
     */
    Grammar build_grammar() {
        // Contains all rule intervals that contain the current index from least to most deeply nested
        std::vector<RuleInterval *> nesting_stack;

        // Contains the tentative symbol list of the rule corresponding to the interval at the same
        // position in nesting_stack.
        std::vector<std::vector<len_t>> symbol_stack;
        Grammar                         gr(m_num_rules);
        gr.set_start_rule_id(0);

        for (size_t i = 0; i < m_underlying.size(); ++i) {
            // If the index moved past the current rule interval,
            // remove all intervals which ended and add the resulting rules to the grammar
            while (!nesting_stack.empty() && i > m_interval_index.end(*nesting_stack.back())) {
                const size_t id = nesting_stack.back()->rule_id();
                nesting_stack.pop_back();
                gr.set_rule(id, std::move(symbol_stack.back()));
                symbol_stack.pop_back();

                if (!symbol_stack.empty()) {
                    symbol_stack.back().push_back(id + Grammar::RULE_OFFSET);
                }
            }

            // If new rules start at this index, add them to the stack to designate them as more deeply nested rule
            auto interval_index_at = m_interval_index.deepest_interval_index_at(i);
            if (interval_index_at != INVALID) {
                RuleInterval *interval_at = &m_interval_index.get(interval_index_at);
                for (interval_index_at = interval_at->first_at_start_index(); interval_index_at != INVALID;
                     interval_index_at = interval_at->next_at_start_index()) {
                    interval_at = &m_interval_index.get(interval_index_at);
                    nesting_stack.push_back(interval_at);
                    symbol_stack.emplace_back();
                }
            }

            // Add the current literal to the most deeply nested rule's stack
            tdc::uliteral_t c = m_underlying[i];
            symbol_stack.back().push_back(c);
        }

        while (!nesting_stack.empty()) {
            const size_t id = nesting_stack.back()->rule_id();
            nesting_stack.pop_back();
            gr.set_rule(id, std::move(symbol_stack.back()));
            symbol_stack.pop_back();

            if (!symbol_stack.empty()) {
                symbol_stack.back().push_back(id + Grammar::RULE_OFFSET);
            }
        }

        return gr;
    }
};

} // namespace tdc::grammar::areacomp
