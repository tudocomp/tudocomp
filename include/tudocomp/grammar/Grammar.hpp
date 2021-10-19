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
    size_t m_max_id;

public:
    
    static const size_t RULE_OFFSET = 256; 

    Grammar() : m_rules{}, m_max_id{0} {}
    
    symbols_t& operator[](const size_t id) {
        return m_rules[id];
    }

    std::map<size_t, symbols_t> &operator*() {
        return m_rules;
    }

    const size_t max_id() const {
        return m_max_id;
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

};


}}