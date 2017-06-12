#pragma once

#include <tudocomp/meta/ast/Acceptor.hpp>

#include <tudocomp/meta/ast/Node.hpp>
#include <tudocomp/meta/ast/Value.hpp>
#include <tudocomp/meta/ast/List.hpp>
#include <tudocomp/meta/ast/Object.hpp>

#include <stdexcept>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Accepts whitespace characters.
constexpr Acceptor sym_whitespace = Acceptor(" \t\n\r");

/// \brief Accepts ANSI letters (A-Z and a-z).
constexpr Acceptor sym_letter = Acceptor(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

/// \brief Accepts ANSI digits (0-9).
constexpr Acceptor sym_digit = Acceptor("0123456789");

/// \brief Accepts special characters allowed in names.
constexpr Acceptor sym_name_special = Acceptor("_");

/// \brief Accepts characters allowed in names.
constexpr UnionAcceptor sym_name = UnionAcceptor({
    sym_letter, sym_digit, sym_name_special});

/// \brief Error type for parsing related errors.
class ParseError : public std::runtime_error {
public:
    /// \brief Main constructor.
    /// \param err the error message
    /// \param pos the current position in the parsed string
    inline ParseError(const std::string& err, size_t pos)
        : std::runtime_error(err + " (pos: " + std::to_string(pos+1) + ")") {
    }
};

/// \brief Provides functionality to parse a string into an AST.
class Parser {
private:
    std::string m_str;
    size_t m_cursor;

    inline Parser(const std::string& str) : m_str(str),
                                            m_cursor(0) {
    }

    inline char peek() const {
        return m_cursor < m_str.length() ? m_str[m_cursor] : 0;
    }

    inline size_t parse_whitespace() {
        // parse whitespace
        size_t count = 0;
        while(sym_whitespace.accept(peek())) {
            ++count;
            ++m_cursor;
        }
        return count;
    }

    inline std::shared_ptr<Value> parse_string_literal() {
        // save enclosing character (e.g. quote or double quote)
        const char enclose = peek();
        ++m_cursor; // quote

        // save starting position
        size_t start = m_cursor;

        // find closing quote
        auto end = m_str.find(enclose, m_cursor);

        if(end == std::string::npos) {
            throw ParseError("string literal not closed", m_cursor);
        }

        m_cursor = end + 1;

        // yield substring
        return std::make_shared<Value>(
            m_str.substr(start, m_cursor - 1 - start));
    }

    inline std::shared_ptr<Value> parse_numeric_literal() {
        // save starting position
        size_t start = m_cursor;

        // parse sign, if any
        if(peek() == '+' || peek() == '-') ++m_cursor;

        // parse digits or at most one fraction separator (dot)
        bool fraction = false;
        while(sym_digit.accept(peek()) || peek() == '.') {
            if(peek() == '.') {
                if(fraction) {
                    throw ParseError(
                        "numeric values may have at most one fraction",
                        m_cursor);
                } else {
                    fraction = true;
                }
            }

            ++m_cursor;
        }

        // yield substring
        return std::make_shared<Value>(
            m_str.substr(start, m_cursor - start));
    }

    inline std::shared_ptr<List> parse_list() {
        auto list = std::make_shared<List>();

        if(peek() != '[') throw ParseError("unexpected list opening", m_cursor);

        ++m_cursor; // opening bracket
        parse_whitespace();
        while(peek() != ']') {
            // parse values separated by commas
            auto node = parse_node();
            if(!node) throw ParseError("unexpected end of input", m_cursor);

            list->add_value(node);

            parse_whitespace();
            if(peek() == ',') {
                ++m_cursor; // comma
                parse_whitespace();
            } else if(peek() != ']') {
                throw ParseError("invalid list syntax", m_cursor);
            }
        }
        ++m_cursor; // closing bracket

        return list;
    }

    inline std::string parse_name() {
        // save start position
        size_t start = m_cursor;

        // require a letter to start the name
        if(sym_letter.accept(peek())) {
            // parse name symbols
            while(sym_name.accept(peek())) ++m_cursor;
        }

        // yield substring
        return m_str.substr(start, m_cursor - start);
    }

    inline void parse_param_list(std::shared_ptr<Object> obj) {
        if(peek() == '(') {
            // param list
            ++m_cursor; // opening paranthesis
            parse_whitespace();

            while(peek() != ')') {
                // attempt to parse a name first
                auto name = parse_name();
                if(name.empty()) {
                    // treat as nameless value
                    auto node = parse_node();
                    if(!node) {
                        throw ParseError("unexpected end of input", m_cursor);
                    }
                    obj->add_param(Param(node));
                } else {
                    parse_whitespace();

                    // peek next character and decide what to do
                    auto c = peek();
                    if(c == '=') {
                        ++m_cursor;
                        parse_whitespace();

                        // parameter
                        auto node = parse_node();
                        if(!node) {
                            throw ParseError("unexpected end of input", m_cursor);
                        }
                        obj->add_param(Param(name, node));
                    } else {
                        // treat as a nameless sub object
                        auto sub = std::make_shared<Object>(name);
                        parse_param_list(sub); // recursion!
                        obj->add_param(Param(sub));
                    }
                }
                if(peek() == ',') {
                    ++m_cursor; // comma
                    parse_whitespace();
                } else if(peek() != ')') {
                    throw ParseError("invalid param list syntax", m_cursor);
                }
            }
            ++m_cursor; // closing paranthesis
        }
    }

    inline std::shared_ptr<Object> parse_object() {
        // attempt to parse a name
        auto name = parse_name();
        if(name.empty()) {
            return std::shared_ptr<Object>(); // null
        } else {
            auto obj = std::make_shared<Object>(name);
            parse_whitespace();
            parse_param_list(obj);
            return obj;
        }
    }

    inline std::shared_ptr<Node> parse_node() {
        parse_whitespace();

        // peek next character and decide what to do
        auto c = peek();
        if(c == 0) {
            // end
            return std::shared_ptr<Node>();
        } else if(c == '\'' || c == '\"') {
            // string literal, enclosed by quotes or double quotes
            return parse_string_literal();
        } else if(c == '+' || c == '-' || sym_digit.accept(c)) {
            // numeric literal
            return parse_numeric_literal();
        } else if(c == '[') {
            // a list of nodes
            return parse_list();
        } else {
            // attempt to parse an object
            auto obj = parse_object();
            if(!obj) {
                throw ParseError("syntax error", m_cursor);
            } else {
                return obj;
            }
        }
    }

public:
    /// \brief Parses the given string and returns the resulting AST.
    /// \param str the input string
    /// \return the resulting AST
    inline static std::shared_ptr<Node> parse(const std::string& str) {
        return Parser(str).parse_node();
    }
};

}}} //ns
