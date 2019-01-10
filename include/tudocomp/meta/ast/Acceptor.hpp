#pragma once

#include <cstring>
#include <initializer_list>
#include <tudocomp/util/conststr.hpp>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Implements a predicate for accepting certain characters.
///
/// This is meant to use by a parser to decide whether a read character belongs
/// to a group of expected characters and can be successfully parsed.
class Acceptor {
private:
    conststr m_symbols;

public:
    /// \brief Main constructor.
    /// \param symbols the string of characters to accept
    inline constexpr Acceptor(conststr symbols)
        : m_symbols(symbols) {
    }

    /// \brief Tests if the given character belongs to the group of accepted
    ///        characters.
    /// \param c the character in question
    /// \return \c true if accepted, \c false otherwise
    inline constexpr bool accept(const char c) const {
        for(size_t i = 0; i < m_symbols.size(); i++) {
            if(c == m_symbols[i]) return true;
        }
        return false;
    }
};

}}} //ns
