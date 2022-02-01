#pragma once

#include <cstdio>
#include <map>
#include <unordered_map>
#include <vector>

#include <tudocomp/grammar/GrammarCoding.hpp>

namespace tdc {
namespace grammar {

/**
 * @brief A representation of a Grammar as a map, with rule ids as keys and vectors of unsiged integers as symbol
 * containers
 */
class Grammar {

  public:
    using Symbols = std::vector<size_t>;

  private:
    /**
     * @brief The map keeping the grammar's rules.
     *
     * This map maps from the rule's id to the sequence of symbols in its right side.
     * The symbols are either the code of the character, if the symbol is a literal, or the
     * id of the rule the nonterminal belongs to offset by 256, if the symbol is a nonterminal.
     *
     * Therefore, if the value 274 is read from the symbols vector, it means that this is a nonterminal, since it is >=
     * 256. It is the nonterminal corresponding to the rule with id `274 - 256 = 18`.
     *
     * Note, that this only applies to the symbols in the vector and not to the keys of the map.
     */
    std::unordered_map<size_t, Symbols> m_rules;

    /**
     * @brief The id of the start rule
     */
    size_t m_start_rule_id;

  public:
    /**
     * @brief The value by which rule ids are offset in the symbol vectors of rules.
     * The values 0 to 255 (extended ASCII) are used for terminals, while values starting at 256 are used for rule ids.
     * The value 256 refers to rule 0, 257 refers to rule 1 et cetera.
     */
    static const size_t RULE_OFFSET = 256;

    /**
     * @brief Construct an empty Grammar with a start rule of 0
     *
     */
    Grammar() : m_rules{}, m_start_rule_id{0} {}

    /**
     * @brief Accesses the symbols vector for the rule of the given id
     *
     * @param id The rule id whose symbols vector to access
     * @return Symbols& A reference to the rule's symbols vector
     */
    Symbols &operator[](const size_t id) { return m_rules[id]; }

    /**
     * @brief Appends a terminal to the given rule's symbols vector
     *
     * @param id The id of the rule whose symbols vector should be appended to
     * @param symbol The terminal symbol to append to the vector. Note that this value should be lesser than RULE_OFFSET
     */
    void append_terminal(const size_t id, size_t symbol) { m_rules[id].push_back(symbol); }

    /**
     * @brief Appends a non terminal to the given rule's vector
     *
     * @param id The id of the rule whose symbols vector should be appended to
     * @param rule_id The id of the rule, whose non terminal should be appended to the vector. Note, that the parameter
     * should not have RULE_OFFSET added to it when passing it in.
     */
    void append_nonterminal(const size_t id, size_t rule_id) { append_terminal(id, rule_id + Grammar::RULE_OFFSET); }

    /**
     * Set the right side of a rule directly. Note that this will overwrite the previous mapping
     *
     * @param id The id of the rule whose right side to set
     * @param symbols The container of the right side's symbols
     */
    void set_rule(const size_t id, Symbols &&symbols) { m_rules[id] = symbols; }

    /**
     * @brief Renumbers the rules in the grammar in such a way that rules with index i only depend on rules with indices
     * lesser than i.
     *
     */
    void dependency_renumber() {
        std::unordered_map<size_t, size_t> renumbering;
        size_t                             count = 0;

        // Calculate a renumbering
        std::function<void(size_t)> renumber = [&](size_t rule_id) {
            Symbols &symbols = m_rules[rule_id];
            for (auto &symbol : symbols) {
                if (is_terminal(symbol) || renumbering.contains(symbol - RULE_OFFSET))
                    continue;
                renumber(symbol - RULE_OFFSET);
            }
            renumbering[rule_id] = count++;
        };
        renumber(m_start_rule_id);
        // make count equal to the max. id
        count--;

        // renumber the rules and the nonterminals therein
        std::unordered_map<size_t, Symbols> new_rules;
        for (auto &[old_id, symbols] : m_rules) {
            // Renumber all the nonterminals in the symbols vector
            for (auto &&symbol : symbols) {
                if (is_terminal(symbol))
                    continue;
                symbol = (renumbering[symbol - RULE_OFFSET]) + RULE_OFFSET;
            }
            // Put assign the rule to its new id
            new_rules[renumbering[old_id]] = std::move(symbols);
        }

        m_rules         = std::move(new_rules);
        m_start_rule_id = count;
    }

    /**
     * @brief Prints the grammar to an output stream.
     *
     * @param out An output stream to print the grammar to
     */
    void print(std::ostream &out = std::cout) {
        std::map<size_t, const Symbols *> ordered;
        for (const auto &[id, symbols] : m_rules) {
            ordered[id] = &symbols;
        }
        for (const auto &[id, symbols] : ordered) {
            out << 'R' << id << " -> ";
            for (auto &symbol : *symbols) {
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
    std::string reproduce() {
        dependency_renumber();

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
                if (is_terminal(symbol)) {
                    ss << (char) symbol;
                } else {
                    ss << expansions[symbol - RULE_OFFSET];
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
     * @brief Set the start rule id
     *
     * @param i The id the start rule id should be set to
     */
    void set_start_rule_id(size_t i) { m_start_rule_id = i; }

    /**
     * @brief Calculates the size of the grammar
     *
     * The size is defined as the count of symbols in all right sides of this grammar's rules
     *
     * @return const size_t The size of the grammar
     */
    const size_t grammar_size() const {
        auto count = 0;
        for (const auto &[rule_id, symbols] : m_rules) {
            count += symbols.size();
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
    const bool contains_rule(int id) const { return m_rules.find(id) != m_rules.end(); }

    /**
     * @brief Checks whether the grammar is empty
     *
     * @return true If the grammar contains no rules
     * @return false If the grammar contains rules
     */
    const bool empty() const { return rule_count() == 0; }

    /**
     * @brief Checks whether a symbol is a terminal.
     *
     * A symbol is considered a terminal if its value falls into the extended ASCII range, i.e. it is between 0 and 255.
     *
     * @see RULE_OFFSET
     *
     * @param symbol The symbol to check
     * @return true If the symbol is in the extended ASCII range
     * @return false If the symbol is not in the extended ASCII range
     */
    static const bool is_terminal(size_t symbol) { return symbol < RULE_OFFSET; }

    /**
     * @brief Checks whether a symbol is a non-terminal.
     *
     * A symbol is considered a non-terminal if its value is outside the extended ASCII range, i.e. it is greater or
     * equal to than 256.
     *
     * @see RULE_OFFSET
     *
     * @param symbol The symbol to check
     * @return true If the symbol outside the extended ASCII range
     * @return false If the symbol is in the extended ASCII range
     */
    static const bool is_non_terminal(size_t symbol) { return !is_terminal(symbol); }

    auto begin() const { return m_rules.cbegin(); }

    auto end() const { return m_rules.cend(); }

    /**
     * @brief Creates a sorted map containing the same data as in @link m_rules, but containing pointers to the vectors
     * instead.
     *
     * This is needed for some encoders.
     */
    std::map<size_t, Symbols *> rules_sorted() {
        std::map<size_t, Symbols *> map;
        for (auto &[id, symbols] : m_rules) {
            map[id] = &symbols;
        }
        return map;
    }
};

} // namespace grammar
} // namespace tdc
