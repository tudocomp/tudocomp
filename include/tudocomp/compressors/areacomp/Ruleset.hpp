#pragma once

#include <ranges>
#include <string>
#include <vector>
#include <unordered_set>
#include <queue>
#include <tudocomp/compressors/areacomp/RuleIntervalIndex.hpp>
#include <tudocomp/compressors/areacomp/areafunctions/AreaFunction.hpp>

namespace tdc::grammar::areacomp {

    template<size_t sampling>
    class Ruleset {

        static const size_t INVALID = std::numeric_limits<size_t>::max();

        const io::InputView &m_underlying;
        RuleIntervalIndex<sampling> m_interval_index;

        size_t m_num_rules;


    public:
        Ruleset(io::InputView &underlying) : m_underlying{underlying}, m_interval_index{0, underlying.size()},
                                             m_num_rules{1} {}


    private:
        size_t next_rule_id() {
            return m_num_rules++;
        }

        /**
         * Checks whether the given Interval can be substituted without violating other previous substitutions in this area.
         * In essence, this checks whether there is an already substituted interval in which a start symbol and and end symbol can be found,
         * which correspond to the indices from and to in the input String.
         *
         * For example. Take the following grammar for the string "abacaba":
         *
         * S -> AcA
         * A -> aba
         *
         * This method returns true for the input [0, 3]
         * This is because the 0 index here corresponds to either the first "A" in Rule S or the first "a" in Rule A and the 3 index corresponds to the "c", both in Rule S.
         * Here, a start- and end symbol can be found that are in the same rule interval and correspond to the indices. These being the "A" and "c" in Rule S respectively.
         *
         * This method returns false for the input [1, 3], since the only symbol that corresponds to the index 1 is the "b" in rule A, and for index 3 it is only the "c".
         * So there is no way to find a start- and end symbol which both lie in the same substituted interval.
         *
         * This method also returns false for the input [0, 5]. Index 0 corresponds to the first "A" in rule S and the first "a" in rule A, while index 5 corresponds only to the "b" in rule A.
         * While start- and end symbols can be found, which are of the same rule ("a" and "b" of rule A), they are *not* of the same substituted interval.
         * While the "a" is of the first substituted instance of rule A, the "b" is of the second substituted instance. As a result this substitution would also be invalid.
         *
         * @param from The lower bound of the range (inclusive)
         * @param to The upper bound of the range (inclusive)
         * @return true, if the interval starts in the same rule range as it started. false otherwise
         */
        bool substitution_allowed(size_t from, size_t to) {
            auto from_interval = m_interval_index.interval_containing(from);

            // If the start index is the start of this interval, it might imply that this is a non-terminal in a less-deeply nested rule.
            // In that case, we iterate upwards by using the parent pointers in order to find the first interval, which contains the end index also.
            // Mind, that only intervals that start at the same index, and the parent of the last such interval can be considered here.
            // This is because while the start index is equal to the current interval's start position, that means that the start index
            // describes the position of a non-terminal in its parent interval.
            while (from == from_interval->start() && to > from_interval->end() && from_interval->has_parent()) {
                from_interval = from_interval->parent();
            }

            // If there is no such interval, that would also contain "to", the substitution can't be allowed
            if (to > from_interval->end()) {
                return false;
            }

            auto to_interval = m_interval_index.interval_containing(to);
            while (!to_interval->contains(*from_interval) && to == to_interval->end() && to_interval->has_parent()) {
                to_interval = to_interval->parent();
            }

            return from_interval == to_interval;
        }

        /**
         * Determines, whether there are multiple Occurrences of the same pattern in distinct contexts regarding already existing rules
         * For example, the string "aaaaaa" and the grammar
         * R0 -> R1 R1
         * R1 -> aaa
         * The algorithm might try to replace the pattern "aa" at positions 0, 2 and 4, since it only computed the Suffix- and LCP-Array in the beginning.
         * Since in the algorithm, pattern occurrences, that cross the boundaries of pre-existing rules are removed before this algorithm is called,
         * the occurrence at position 2 is removed. This is the case, because it would cross the boundary from the first R1 range into the second R1 range,
         * if you look at the right side of R0.
         * But even so, factoring out "aa" at index 0 and 4 is not possible, since the pattern would appear once as the first "aa" in R1
         * and once as the second "aa" in R1. This method detects, if the given positions cannot be factored out.
         *
         * @param positions The positions to check
         * @return true, if there are positions that can be factored out, false otherwise
         */
        bool differing_occurrences(std::vector<size_t> &positions) {
            if (positions.empty()) {
                return false;
            }

            std::unordered_set<size_t> set;
            size_t first_rule_id = INVALID;

            for (auto pos: positions) {
                if (pos == INVALID) {
                    continue;
                }

                auto rule_interval = m_interval_index.interval_containing(pos);
                auto rule_id = rule_interval->rule_id();
                auto start_index = rule_interval->start();

                if (first_rule_id == INVALID) {
                    first_rule_id = rule_id;
                } else if (rule_id != first_rule_id || set.contains(start_index)) {
                    // This interval (or at least one that starts at the same index) already has an interval.
                    // This new one must be distinct from that other one. If it's the very same interval, that is obvious.
                    // If this is a different interval from before, it can't be the same rule id, since then it would have a later start id
                    // Therefore the occurrences must be distinct also
                    return true;
                }
                set.insert(start_index);
            }
            return false;
        }

        /**
        * Filters out the occurrences of the pattern with length len and at the given positions, which start in one rule range and end in another.
        * Also filters out overlapping occurrences.
        * @param positions The positions to filter
        * @param len The length of the pattern
        * @return The amount of valid positions
        *
        * @see #substitution_allowed(size_t, size_t)
        */
        size_t clean_positions(std::vector<size_t> &positions, size_t len) {
            size_t count = 0;
            size_t previous = INVALID;
            for (unsigned long &pos: positions) {
                if ((previous == INVALID || previous + len <= pos) && substitution_allowed(pos, pos + len - 1)) {
                    previous = pos;
                    count++;
                } else {
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
            if (len <= 1 || positions.size() < 2) return;
            const size_t next_id = next_rule_id();

            // Iterate through the positions and substitute each occurrence
            for (auto position : positions) {
                if (position == INVALID) {
                    continue;
                }

                m_interval_index.mark(next_id, position, position + len - 1);
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
        template<typename area_fun_t, typename ds_t>
            //requires AreaFun<area_fun_t, ds_t>
        void compress(area_fun_t fun, ds_t text_ds) {

            const auto &sa = text_ds.template get<ds::SUFFIX_ARRAY>();
            const auto &lcp = text_ds.template get<ds::LCP_ARRAY>();

            StatPhase global_phase("Build Child Array");

            areacomp::ChildArray<> cld(lcp, m_underlying.size());

            global_phase.split("Priority Queue");

            auto queue = AreaData::queue();

            for (auto interval : cld.get_lcp_intervals(2)) {
                AreaData data = fun.area(text_ds, cld, interval.start + 1, interval.end);
                if (data.area() > 0) {
                    queue.push(data);
                }
            }

            global_phase.split("Compression Loop");

            while (!queue.empty()) {
                //StatPhase sub_phase("Get best area");
                // Poll the best interval
                auto area_data = queue.top();

                //sub_phase.split("Get positions");

                // The positions at which the pattern can be found
                std::vector<size_t> positions;
                positions.reserve(area_data.high() - area_data.low() + 2);


                for (size_t i = area_data.low() - 1; i <= area_data.high(); ++i) {
                    positions.push_back(sa[i]);
                }

                // Get the length of the longest common prefix in this range of the lcp array
                // This will be the length of the pattern that is to be replaced.
                const size_t len = area_data.len();

                // This means there is no repeated subsequence of 2 or more characters. In this case, abort
                if (len <= 1) {
                    break;
                }

                //sub_phase.split("Sort positions");

                std::sort(positions.begin(), positions.end());


                //sub_phase.split("Clean positions");
                // Remove positions at which substituting is impossible
                const size_t position_count = clean_positions(positions, len);
                if (position_count <= 1) {
                    queue.pop();
                    continue;
                }

                //sub_phase.split("Differing Occurrences");
                // Check, if the found occurrences are actually distinct in the current grammar
                const bool multiple_occurrences = differing_occurrences(positions);

                // If there is only one occurrence left, there is no use in creating a rule in the grammar
                if(!multiple_occurrences) {
                    queue.pop();
                    continue;
                }

                //sub_phase.split("Substitute");

                substitute(positions, len);
                queue.pop();
            }

        }

        RuleIntervalIndex<sampling> &interval_index() {
            return m_interval_index;
        }

        std::string interval_index_to_string() {
            std::vector<std::string> buf;

            auto current = interval_index().floor_interval(interval_index().text_len() - 1);

            while (current != nullptr) {
                std::stringstream ss;
                ss << current->start() << ": [";
                auto current_vertical = current->first_at_start_index();
                while (true) {
                    ss << current_vertical->rule_id() << ": " << current_vertical->end();
                    if(current_vertical->next_at_start_index() == nullptr) {
                        ss << "]";
                        buf.push_back(ss.str());
                        break;
                    } else {
                        ss << ", ";
                        current_vertical = current_vertical->next_at_start_index();
                    }
                }

                if (current->start() == 0) {
                    break;
                }

                current = interval_index().floor_interval(current->start() - 1);
            }

            std::stringstream ss;

            std::reverse(buf.begin(), buf.end());
            for(const auto& str : buf) {
                ss << str << ", ";
                //std::cout << str << std::endl;
            }

            return ss.str();
        }

        Grammar build_grammar() {
            // Contains all rule intervals that contain the current index from least to most deeply nested
            std::vector<std::shared_ptr<RuleInterval>> nesting_stack;

            // Contains the tentative symbol list of the rule corresponding to the interval at the same
            // position in nesting_stack.
            std::vector<std::vector<size_t>> symbol_stack;
            Grammar gr;
            gr.set_start_rule_id(0);

            for (size_t i = 0; i < m_underlying.size(); ++i) {
                // If the index moved past the current rule interval,
                // remove all intervals which ended and add the resulting rules to the grammar
                while (!nesting_stack.empty() && i > nesting_stack.back()->end()) {
                    const size_t id = nesting_stack.back()->rule_id();
                    nesting_stack.pop_back();
                    gr.set_rule(id, std::move(symbol_stack.back()));
                    symbol_stack.pop_back();

                    if (!symbol_stack.empty()) {
                        symbol_stack.back().push_back(id + Grammar::RULE_OFFSET);
                    }
                }

                // If new rules start at this index, add them to the stack to designate them as more deeply nested rule
                auto interval_at = m_interval_index.interval_at_start_index(i);
                if(interval_at != nullptr) {
                    for (interval_at = interval_at->first_at_start_index(); interval_at != nullptr; interval_at = interval_at->next_at_start_index()) {
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



}