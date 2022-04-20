#pragma once

#include "tudocomp/ds/IntVector.hpp"
#include "tudocomp/util/bits.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <queue>
#include <tudocomp/compressors/areacomp/Consts.hpp>
#include <tudocomp/def.hpp>
#include <tudocomp/grammar/Grammar.hpp>
#include <tuple>
#include <vector>

namespace tdc::grammar {

template<size_t sampling = 6400>
class SampledScanQueryGrammar {

  public:
    using Symbols = std::vector<len_t>;

  private:
    /**
     * @brief The vector keeping the grammar's rules.
     *
     * This vector maps from the rule's id to the sequence of symbols in its right side.
     * The symbols are either the code of the character, if the symbol is a literal, or the
     * id of the rule the nonterminal belongs to offset by 256, if the symbol is a nonterminal.
     *
     * Therefore, if the value 274 is read from the symbols vector, it means that this is a nonterminal, since it is >=
     * 256. It is the nonterminal corresponding to the rule with id `274 - 256 = 18`.
     *
     * Note, that this only applies to the symbols in the innner vector and not to the indexes of the outer vector.
     */
    std::vector<Symbols> m_rules;

    /**
     * @brief The id of the start rule
     */
    size_t m_start_rule_id;

    len_t            m_start_rule_full_length;
    DynamicIntVector m_full_lengths;

    struct QuerySample {

        /**
         * @brief The rule id of the deepest rule that contains the entire block
         */
        len_t lowest_interval_containing_block;

        /**
         * @brief The index of the symbol in the above rule which is the first nonterminal that lies in the block
         */
        uint16_t internal_index_of_first_in_block;

        /**
         * @brief The ìndex in the source string at which the expansion of the above symbol starts
         */
        uint16_t relative_index_in_block;

        QuerySample() :
            lowest_interval_containing_block{0},
            internal_index_of_first_in_block{std::numeric_limits<uint16_t>().max()},
            relative_index_in_block{std::numeric_limits<uint16_t>().max()} {}

        QuerySample(len_t lowest_interval, uint16_t internal_first_index, uint16_t relative_index) :
            lowest_interval_containing_block{lowest_interval},
            internal_index_of_first_in_block{internal_first_index},
            relative_index_in_block{relative_index} {}
    };

    std::vector<QuerySample> m_samples;

    void calculate_samples() {
        // Start index, rule_id
        std::queue<std::pair<size_t, size_t>> rule_queue;

        auto sample_count = (m_start_rule_full_length + sampling - 1) / sampling;
        m_samples         = std::vector<QuerySample>(sample_count);

        // Whether the sample of the corresponding index has been changed and the internal indexes need to be updated
        std::vector<bool> dirty_samples(sample_count);
        std::fill(dirty_samples.begin(), dirty_samples.end(), false);

        rule_queue.emplace(0, m_start_rule_id);

        while (!rule_queue.empty()) {
            const auto [start_index, rule_id] = rule_queue.front();
            rule_queue.pop();
            const auto expanded_rule_len = rule_length(rule_id);

            // The indexes of the first and last blocks that this rule fully covers
            const auto first_full_block = (start_index + sampling - 1) / sampling;
            const auto last_full_block  = (start_index + expanded_rule_len) / sampling;
            // The weird term in the break condition is because of the last block most likely being smaller than the
            // others If our rule spans an interval that ends at the last character of our source string, we need to
            // include the last incomplete block too
            for (size_t i = first_full_block;
                 i < last_full_block + (start_index + expanded_rule_len == m_start_rule_full_length);
                 i++) {
                // Since we use a queue, this is the deepest rule we have seen so far
                m_samples[i].lowest_interval_containing_block = rule_id;
                // We modified the sample
                dirty_samples[i] = true;
            }

            auto idx_in_source = start_index;
            auto internal_idx  = 0;

            Symbols &symbols = m_rules[rule_id];
            for (auto symbol : symbols) {
                // If we modified our sample before, we need to update which rule is the first that starts inside it
                auto         sample_idx = idx_in_source / sampling;
                QuerySample &sample     = m_samples[sample_idx];
                if (dirty_samples[sample_idx]) {
                    sample.relative_index_in_block          = idx_in_source % sampling;
                    sample.internal_index_of_first_in_block = internal_idx;
                    dirty_samples[sample_idx]               = false;
                }
                if (Grammar::is_terminal(symbol)) {
                    idx_in_source++;
                } else {
                    // Add this nonterminal to the queue to be processed later
                    rule_queue.emplace(idx_in_source, symbol - Grammar::RULE_OFFSET);
                    idx_in_source += rule_length(symbol - Grammar::RULE_OFFSET);
                }
                internal_idx++;
            }
        }
    }

    std::pair<len_t, DynamicIntVector> calculate_full_lengths() {

        DynamicIntVector full_lengths(m_rules.size(), 0, sizeof(len_t) * 8);

        size_t max_len = 0;

        for (size_t i = 0; i < m_rules.size(); i++) {
            auto &symbols = m_rules[i];
            for (auto symbol : symbols) {
                if (Grammar::is_terminal(symbol)) {
                    full_lengths[i]++;
                } else {
                    full_lengths[i] += full_lengths[symbol - Grammar::RULE_OFFSET];
                }
            }
            // We do not want to include the length of the start rule, since we will save it separately
            if (i < m_rules.size() - 1) {
                max_len = std::max(max_len, full_lengths[i] + 0);
            }
        }
        size_t start_rule_full_length = full_lengths.back();
        full_lengths.pop_back();
        full_lengths.width(bits_for(max_len));
        full_lengths.shrink_to_fit();
        return {start_rule_full_length, full_lengths};
    }

  public:
    /**
     * @brief Construct an empty Grammar with a start rule of 0 and with a given capacity.
     *
     * @param capacity The number of rules the underlying vector will be setup to hold
     */
    SampledScanQueryGrammar(Grammar &&other) {
        other.dependency_renumber();

        m_start_rule_id = other.start_rule_id();
        m_rules         = Grammar::consume(std::move(other));

        auto [start_rule_full_length, full_lengths] = calculate_full_lengths();
        m_start_rule_full_length                    = start_rule_full_length;
        m_full_lengths                              = std::move(full_lengths);
        calculate_samples();
    }

    /**
     * @brief Accesses the symbols vector for the rule of the given id
     *
     * @param id The rule id whose symbols vector to access
     * @return Symbols& A reference to the rule's symbols vector
     */
    const Symbols &operator[](const size_t id) const { return m_rules[id]; }

    /**
     * @brief Prints the grammar to an output stream.
     *
     * @param out An output stream to print the grammar to
     */
    void print(std::ostream &out = std::cout) const {
        for (size_t id = 0; id < m_rules.size(); id++) {
            const Symbols &symbols = m_rules[id];
            out << 'R' << id << " -> ";
            for (auto symbol : symbols) {
                if (Grammar::is_terminal(symbol)) {
                    auto c = (char) symbol;
                    switch (c) {
                        case '\n': {
                            out << "\\n";
                            break;
                        }
                        case '\r': {
                            out << "\\r";
                            break;
                        }
                        case '\t': {
                            out << "\\t";
                            break;
                        }
                        case '\0': {
                            out << "\\0";
                            break;
                        }
                        case ' ': {
                            out << '_';
                            break;
                        }
                        default: {
                            out << c;
                        }
                    }
                } else {
                    out << 'R' << symbol - Grammar::RULE_OFFSET;
                }
                out << ' ';
            }
            out << std::endl;
        }
    }

    /**
     * @brief Reproduces this grammar's source string
     *
     * @return std::string The source string
     */
    std::string reproduce() const {

        // This vector contains a mapping of a rule id (or rather, a nonterminal) to the string representation of the
        // rule, were it fully expanded
        std::vector<std::string> expansions;

        const auto n = rule_count();

        // After dependency_renumber is called, the rules occupy the ids 0 to n-1 (inclusive)
        for (size_t rule_id = 0; rule_id < n; rule_id++) {
            const auto symbols = m_rules[rule_id];

            // The string representation of the rule currently being processed
            std::stringstream ss;
            for (const auto symbol : symbols) {
                // If the scurrent symbol is a terminal, it can be written to this rule's string representation as is.
                // If it is a nonterminal, it can only be the nonterminal of a rule that has already been fully
                // expanded, since rules which depend on the least amount of other rules are always read first. The
                // string expansion thereof is written to the current rule's string representation
                if (Grammar::is_terminal(symbol)) {
                    ss << (char) symbol;
                } else {
                    ss << expansions[symbol - Grammar::RULE_OFFSET];
                }
            }

            // If we have arrived at the start rule there are no more rules to be read.
            // The string expansion of this rule is obviously the original text.
            if (rule_id == start_rule_id()) {
                return ss.str();
            }

            expansions.push_back(ss.str());
        }
        return "";
    }

    /**
     * @brief Returns the id of this grammar's start rule
     *
     * @return const size_t The id of the start rule
     */
    const size_t start_rule_id() const { return m_start_rule_id; }

    /**
     * @brief Returns the expanded length of the symbol at the given index in the right side of the given rule.
     *
     * @param rule_id The rule id.
     * @param index The symbol's index in the right side of the rule.
     *
     * @return The fully expanded length of the symbol.
     */
    const size_t symbol_length(size_t rule_id, size_t index) const {
        const auto symbol = m_rules[rule_id][index];
        return Grammar::is_terminal(symbol)
                   ? 1
                   : (symbol - Grammar::RULE_OFFSET != m_start_rule_id ? m_full_lengths[symbol - Grammar::RULE_OFFSET]
                                                                       : m_start_rule_full_length);
    }

    /**
     * @brief Returns the expanded length of the rule with the given id.
     *
     * @param rule_id The rule's id.
     *
     * @return The fully expanded length of the rule.
     */
    const size_t rule_length(size_t rule_id) const {
        return rule_id != m_start_rule_id ? m_full_lengths[rule_id] : m_start_rule_full_length;
    }

    /**
     * @brief Calculates the size of the grammar
     *
     * The size is defined as the count of symbols in all right sides of this grammar's rules
     *
     * @return const size_t The size of the grammar
     */
    const size_t grammar_size() const {
        auto count = 0;
        for (size_t rule_id = 0; rule_id < m_rules.size(); rule_id++) {
            count += m_rules[rule_id].size();
        }
        return count;
    }

    /**
     * @brief Counts the rules in this grammar
     *
     * @return const size_t The rule count
     */
    const size_t rule_count() const { return m_rules.size(); }

    /**
     * @brief Checks whether this grammar contains the rule with the given id
     *
     * @param id The id to search for
     * @return true If the grammar contains the rule with the given id
     * @return false If the grammar does not contain the rule with the given id
     */
    const bool contains_rule(size_t id) const { return m_rules.size() < id && !m_rules[id].empty(); }

    /**
     * @brief Checks whether the grammar is empty
     *
     * @return true If the grammar contains no rules
     * @return false If the grammar contains rules
     */
    const bool empty() const { return rule_count() == 0; }

    auto begin() const { return m_rules.cbegin(); }

    auto end() const { return m_rules.cend(); }

    /**
     * @brief Gets the sampled queries.
     *
     * @return
     */
    const std::vector<QuerySample> &samples() const { return m_samples; }

    /**
     * @brief Returns the length of the source string.
     *
     * @return The length of the source string.
     */
    const size_t source_length() const { return m_start_rule_full_length; }

    /**
     * @brief Returns the character at index i in the source string.
     *
     * This is a naive implementation. Taking O(n) time in the worst case.
     *
     * @param i The index
     *
     * @return The char at the given index in the source string.
     */
    char at(size_t i) const {
        if (i >= m_start_rule_full_length) {
            // TODO index out of bounds error
        }

        // TODO Wenn keine Regel in nem Block anfängt dann sind halt keine vernünftigen Werte in den Samples
        // Dann gehts kaputt

        const auto         sample_idx = i / sampling;
        const QuerySample &sample     = m_samples[sample_idx];

        // std::cout << "query: i = " << i << std::endl;
        //  forward scan from i inside the block
        if (i >= sample_idx * sampling + sample.relative_index_in_block) {
            size_t rule           = sample.lowest_interval_containing_block;
            size_t internal_index = sample.internal_index_of_first_in_block;
            size_t source_index   = sample_idx * sampling + sample.relative_index_in_block;
            while (source_index < i || Grammar::is_non_terminal(m_rules[rule][internal_index])) {
                const len_t symbol     = m_rules[rule][internal_index];
                const len_t symbol_len = symbol_length(rule, internal_index);
                if (source_index + symbol_len <= i) {
                    source_index += symbol_len;
                    internal_index += 1;
                } else {
                    rule           = symbol - Grammar::RULE_OFFSET;
                    internal_index = 0;
                }
            }
            return (char) m_rules[rule][internal_index];
        } else {
            // backwards scan from before i
            size_t rule           = sample.lowest_interval_containing_block;
            size_t internal_index = sample.internal_index_of_first_in_block;
            // The inclusive end index of the current symbol in the source text
            size_t source_index =
                sample_idx * sampling + sample.relative_index_in_block + symbol_length(rule, internal_index) - 1;
            // We want to scan backwards until we hit i
            // If we hit i, we need to go deeper until we hit a non-terminal
            while (source_index > i || Grammar::is_non_terminal(m_rules[rule][internal_index])) {
                // If this is a terminal, we just skip it
                if (Grammar::is_terminal(m_rules[rule][internal_index])) {
                    internal_index--;
                    source_index--;
                } else {
                    size_t symbol_len = symbol_length(rule, internal_index);
                    if (i + symbol_len <= source_index) {
                        // If it is a nonterminal and i is not inside it, skip it
                        internal_index--;
                        source_index -= symbol_len;
                    } else {
                        // If i is in the nonterminal, we go into the nonterminal
                        rule           = m_rules[rule][internal_index] - Grammar::RULE_OFFSET;
                        internal_index = m_rules[rule].size() - 1;
                    }
                }
            }
            return (char) m_rules[rule][internal_index];
        }
    }
    /**
     * @brief Gets the substring from a start to an end index in the source string.
     *
     * @param pattern_start The inclusive start index.
     * @param pattern_end The exclusive end index.
     *
     * @return The substring in the given interval.
     */
    std::string substr(size_t pattern_start, size_t pattern_end) const {
        if (pattern_start >= source_length() || pattern_end >= source_length()) {
            // TODO out of bounds error
        }
        if (pattern_start >= pattern_end) {
            return "";
        }

        size_t            len = pattern_end - pattern_start;
        std::stringstream ss;

        std::function<void(size_t, size_t)> write = [&](size_t id, size_t start) {
            const Symbols &symbols = m_rules[id];
            size_t         index   = 0;
            {
                // Find symbol start index in this rule
                size_t symbol_len;
                while (start >= (symbol_len = symbol_length(id, index))) {
                    start -= symbol_len;
                    index++;
                }
            }

            // In this case, the symbol at this index is a nonterminal (otherwise we could decrement start if the
            // symbol was a terminal), so we write the non-terminal's contents starting at the start index
            // Also, advance the index by one, sice we handled this index already
            if (start > 0) {
                write(symbols[index++] - Grammar::RULE_OFFSET, start);
            }
            for (; len > 0 && index < symbols.size(); index++) {
                if (Grammar::is_terminal(symbols[index])) {
                    // This is a single terminal. We just write it out
                    ss << (char) symbols[index];
                    len--;
                } else {
                    // Since we're in the midst of writing this rule, we need to start at the first index inside the
                    // nonterminal
                    write(symbols[index] - Grammar::RULE_OFFSET, 0);
                }
            }
        };

        write(start_rule_id(), pattern_start);
        return ss.str();
    }
};

} // namespace tdc::grammar
