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
        len_t internal_index_of_first_in_block;

        /**
         * @brief The ìndex in the source string at which the expansion of the above symbol starts
         */
        len_t relative_index_in_block;

        QuerySample() :
            lowest_interval_containing_block{0},
            internal_index_of_first_in_block{std::numeric_limits<len_t>().max()},
            relative_index_in_block{std::numeric_limits<len_t>().max()} {}

        QuerySample(len_t lowest_interval, len_t internal_first_index, len_t relative_index) :
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
        return Grammar::is_terminal(symbol) ? 1 : rule_length(symbol - Grammar::RULE_OFFSET);
    }

    std::string expansion(size_t rule_id) const {
        auto &symbols = m_rules[rule_id];

        std::ostringstream oss;
        for (auto symbol : symbols) {
            if (Grammar::is_terminal(symbol)) {
                oss << (char) symbol;
                continue;
            }

            oss << expansion(symbol - Grammar::RULE_OFFSET);
        }

        return oss.str();
    }

    /**
     * @brief Returns the expanded length of the rule with the given id.
     *
     * @param rule_id The rule's id.
     *
     * @return The fully expanded length of the rule.
     */
    const size_t rule_length(size_t rule_id) const {
        if (rule_id != m_start_rule_id) {
            return (size_t) m_full_lengths[rule_id];
        } else {
            return m_start_rule_full_length;
        }
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
        const auto         sample_idx = i / sampling;
        const QuerySample &sample     = m_samples[sample_idx];

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
     * @brief Get the intersection of the left part of the block in which substr_start and substr_end are found
     * and the range [substr_start, substr_end).
     *
     * @param substr_start The start of the substring to extract
     * @param substr_end The end of the substring to extract
     * @return std::string The intersection of the substring and the part of the block before (and not including) the
     * sampled position
     */
    std::string scan_left(size_t substr_start, size_t substr_end) const {
        const auto         sample_idx = substr_start / sampling;
        const QuerySample &sample     = m_samples[sample_idx];

        size_t rule           = sample.lowest_interval_containing_block;
        size_t internal_index = sample.internal_index_of_first_in_block - 1;
        size_t source_index   = sample_idx * sampling + sample.relative_index_in_block - 1;

        if (substr_start > source_index || substr_end <= substr_start || sample.relative_index_in_block == 0) {
            // Since the left part of the block is the part up to and not including the sampled position, we have
            // nothing to return if the sampled position
            return "";
        }

        std::ostringstream oss;

        /**
         * @brief Writes the expansion of the rule with the given id to the stringstream in reverse.
         * This respects the range substr_start and substr_end and only writes characters inside that range
         */
        std::function<void(size_t)> write_reverse = [&](size_t id) {
            auto &symbols = m_rules[id];

            for (auto symbol : std::views::reverse(symbols)) {
                if (Grammar::is_terminal(symbol)) {
                    if (source_index < substr_end) {
                        oss << (char) symbol;
                    }
                    source_index--;
                    continue;
                }

                auto rule_len          = rule_length(symbol - Grammar::RULE_OFFSET);
                auto rule_source_index = source_index - rule_len + 1;
                if (rule_source_index < substr_end) {
                    write_reverse(symbol - Grammar::RULE_OFFSET);
                } else {
                    source_index -= rule_len;
                }
            }
        };

        while (source_index >= substr_start) {
            auto symbol              = m_rules[rule][internal_index];
            auto symbol_len          = symbol_length(rule, internal_index);
            auto symbol_source_index = source_index - symbol_len + 1;

            if (symbol_source_index >= substr_end) {
                // The symbol starts after the pattern. we skip it
                source_index -= symbol_len;
                internal_index--;
            } else if (substr_start <= symbol_source_index) {
                // In this case the symbol starts before (or at) the start of the pattern
                if (Grammar::is_terminal(symbol)) {
                    oss << (char) symbol;
                    source_index--;
                } else {
                    write_reverse(symbol - Grammar::RULE_OFFSET);
                    // Write reverse handles modifying the source_index
                }
                internal_index--;
            } else {
                // The symbol starts before the start of the pattern but its expansion lies partly inside the pattern
                rule           = symbol - Grammar::RULE_OFFSET;
                internal_index = m_rules[rule].size() - 1;
            }
        }

        std::string out = oss.str();
        std::ranges::reverse(out.begin(), out.end());
        return out;
    }

    /**
     * @brief Get the intersection of the right part of the block in which substr_start and substr_end are found
     * and the range [substr_start, substr_end). This method only works if the substring denoted by substr_start and
     * substr_end lies entirely in one block
     *
     * @param substr_start The start of the substring
     * @param substr_end The end of the substring (exclusive)
     * @return std::string The intersection of the substring and the part of the block after the sampled position
     */
    std::string scan_right(size_t substr_start, size_t substr_end) const {
        const auto         sample_idx = substr_start / sampling;
        const QuerySample &sample     = m_samples[sample_idx];

        // Get the sampled data in this block
        size_t rule           = sample.lowest_interval_containing_block;
        size_t internal_index = sample.internal_index_of_first_in_block;
        size_t source_index   = sample_idx * sampling + sample.relative_index_in_block;

        if (substr_end <= source_index || substr_end <= substr_start) {
            // Since the right part of the block is the part up to and not including the sampled position, we have
            // nothing to return if the sampled position
            return "";
        }

        std::ostringstream oss;

        /**
         * @brief Writes the expansion of the rule with the given id to the string stream and modifies source_index
         * accordingly. This also respect susbtr_start and substr_end and only writes characters that are in that range
         */
        std::function<void(size_t)> write = [&](size_t id) {
            auto &symbols = m_rules[id];
            for (auto symbol : symbols) {
                if (Grammar::is_terminal(symbol)) {
                    // If this terminal is not inside our range we don't write it
                    if (source_index >= substr_start) {
                        oss << (char) symbol;
                    }
                    source_index++;
                    continue;
                }

                auto rule_len = rule_length(symbol - Grammar::RULE_OFFSET);
                // Does this nonterminal lie completely in our range? then write it. Otherwise just skip it
                if (source_index + rule_len > substr_start) {
                    write(symbol - Grammar::RULE_OFFSET);
                } else {
                    source_index += rule_len;
                }
            }
        };

        while (source_index < substr_end) {
            auto symbol     = m_rules[rule][internal_index];
            auto symbol_len = symbol_length(rule, internal_index);
            auto symbol_end = source_index + symbol_len;

            if (symbol_end <= substr_start) {
                // If this symbol is not in our range, skip it
                source_index += symbol_len;
                internal_index++;
            } else if (symbol_end <= substr_end) {
                // If the symbol is entirely in our range just write it in its entirety
                if (Grammar::is_terminal(symbol)) {
                    oss << (char) symbol;
                    source_index++;
                } else {
                    write(symbol - Grammar::RULE_OFFSET);
                }
                internal_index++;
            } else {
                // The symbol doesn't end before substr_end so we need to go into the symbol and continue searching
                // there
                rule           = symbol - Grammar::RULE_OFFSET;
                internal_index = 0;
            }
        }

        return oss.str();
    }

    /**
     * @brief Gets the substring from a start to an end index in the source string.
     *
     * @param substr_start The inclusive start index.
     * @param substr_len The length of the substring to extract.
     *
     * @return The substring in the given interval.
     */
    std::string substr(size_t substr_start, size_t substr_len) const {

        // Exclusive end index
        const auto substr_end = std::min(substr_start + substr_len, (size_t) m_start_rule_full_length);

        if (substr_end == 0) {
            // So that substr_end - 1 doesn't break everything
            return "";
        }

        // The index of the sample in which the start index lies
        const auto start_sample_idx = substr_start / sampling;
        // The index of the sample in which the (inclusive) end index lies
        const auto end_sample_idx = (substr_end - 1) / sampling;

        std::stringstream ss;

        // Since scan_left and scan_right only work for start- and end-indices which lie in the same block, we call this
        // method once for each block the substring spans over
        if (start_sample_idx != end_sample_idx) {
            for (size_t i = start_sample_idx; i <= end_sample_idx; i++) {
                const auto start_idx = std::max(substr_start, i * sampling);
                // We either need to span the entire block or until the end of the substring, whichever is first
                // For the former case, we need to be aware that if start_idx is not the start of the block, we need to
                // subtract this offset into the block so that the length fits
                const auto len = std::min(sampling - (start_idx - i * sampling), substr_end - start_idx);

                ss << substr(start_idx, len);
            }
            return ss.str();
        }

        // get the left and right halves and put them together
        ss << scan_left(substr_start, substr_end);
        ss << scan_right(substr_start, substr_end);

        return ss.str();
    }
};

} // namespace tdc::grammar
