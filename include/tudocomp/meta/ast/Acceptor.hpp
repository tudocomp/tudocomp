#pragma once

#include <cstring>
#include <initializer_list>

namespace tdc {
namespace meta {
namespace ast {

class Acceptor {
private:
    const char* m_symbols;
    size_t      m_length;

public:
    inline constexpr Acceptor(const char* symbols)
        : m_symbols(symbols),
          m_length(strlen(symbols)) {
    }

    inline constexpr bool accept(const char c) const {
        for(size_t i = 0; i < m_length; i++) {
            if(c == m_symbols[i]) return true;
        }
        return false;
    }
};

class UnionAcceptor {
private:
    std::initializer_list<Acceptor> m_acceptors;

public:
    inline constexpr UnionAcceptor(std::initializer_list<Acceptor> acceptors)
        : m_acceptors(acceptors) {
    }

    inline constexpr bool accept(const char c) const {
        for(auto& acc : m_acceptors) {
            if(acc.accept(c)) return true;
        }
        return false;
    }
};

}}} //ns
