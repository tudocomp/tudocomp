#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <cstdio>

#include <tudocomp/grammar/GrammarCoding.hpp>

namespace tdc {
namespace grammar {

class Grammar {

public:
    using symbols_t = std::vector<size_t>;
    
private:
    /**
     * @brief The map keeping the grammar's rules. 
     * 
     * This map maps from the rule's id to the sequence of symbols in its right side.
     * The symbols are either the code of the character, if the symbol is a literal, or the
     * id of the rule the nonterminal belongs to offset by 256, if the symbol is a nonterminal.
     * 
     * Therefore, if the value 274 is read from the symbols vector, it means that this is a nonterminal, since it is >= 256.
     * It is the nonterminal corresponding to the rule with id `274 - 256 = 18`.
     * 
     * Note, that this only applies to the symbols in the vector and not to the keys of the map.  
     */
    std::map<size_t, symbols_t> m_rules;
    size_t m_start_rule_id;

public:
    
    static const size_t RULE_OFFSET = 256; 

    Grammar() : m_rules{}, m_start_rule_id{0} {}
    
    symbols_t& operator[](const size_t id) {
        return m_rules[id];
    }

    void append_terminal(const size_t id, size_t symbol) {
        m_rules[id].push_back(symbol);
    }
    
    void append_nonterminal(const size_t id, size_t rule_id) {
        append_terminal(id, rule_id + Grammar::RULE_OFFSET);
    }

    const std::map<size_t, symbols_t> &operator*() const {
        return m_rules;
    }
    
    const std::map<size_t, symbols_t> *operator->() const {
        return &m_rules;
    }
    /**
     * @brief Renumbers the rules in the grammar in such a way that Rules with index i only depend on rules with indices lesser than i.  
     * 
     */
    void dependency_renumber() {
        std::map<size_t, size_t> renumbering;
        size_t count = 0;

        std::function<void(size_t)> renumber = [&](size_t rule_id) {
            symbols_t &symbols = m_rules[rule_id];
            for (auto &&symbol : symbols) {
                if (is_terminal(symbol) || renumbering.find(symbol - RULE_OFFSET) != renumbering.end()) continue;
                renumber(symbol - RULE_OFFSET);
            }
            renumbering[rule_id] = count++;
        };
        renumber(m_start_rule_id);
        // make count equal to the max. id
        count--;

        std::map<size_t, symbols_t> new_rules;
        for (auto &rule : m_rules) {
            const auto old_id = rule.first;
            auto &symbols = rule.second;
            // Renumber all the nonterminals in the symbols vector
            for (auto &&symbol : symbols) {
                if (is_terminal(symbol)) continue;
                symbol = (renumbering[symbol - RULE_OFFSET]) + RULE_OFFSET;
            }
            // Put assign the rule to its new id
            new_rules[renumbering[old_id]] = std::move(symbols);
        }

        
        m_rules = std::move(new_rules);
        m_start_rule_id = count - 1;
    }

    void print(std::ostream &out = std::cout) {
        for (auto &&pair : m_rules) {
            const auto id = pair.first;
            const auto symbols = pair.second;
            out << 'R' << id << " -> ";
            for (auto &symbol : symbols) {
                if (Grammar::is_terminal(symbol)) { 
                    out << (char) symbol;
                } else {
                    out << 'R' << symbol - Grammar::RULE_OFFSET;
                }
                out << ' ';
            }
            out << std::endl;
        }   
    }

    const size_t start_rule_id() const {
        return m_start_rule_id;
    }
    
    void set_start_rule_id(size_t i) {
        m_start_rule_id = i;
    }

    const size_t grammar_size() const {
        auto count = 0;
        for (auto &&pair : m_rules) {
            count += pair.second.size();
        }
        return count;
    }

    const size_t rule_count() const {
        return m_rules.size();
    }

    const bool contains_rule(int id) const {
        return m_rules.find(id) != m_rules.end();
    }

    const bool empty() const {
        return rule_count() == 0;
    }
    
    static const bool is_terminal(size_t symbol) {
        return symbol < RULE_OFFSET;
    } 
    
    static const bool is_non_terminal(size_t symbol) {
        return !is_terminal(symbol);
    }
    

};


}}