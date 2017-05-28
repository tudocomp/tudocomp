#pragma once

#include <tudocomp/util/meta/ASTAcceptor.hpp>
#include <tudocomp/util/meta/ASTValue.hpp>
#include <tudocomp/util/meta/ASTPrimitive.hpp>
#include <tudocomp/util/meta/ASTList.hpp>
#include <tudocomp/util/meta/ASTNode.hpp>

#include <stdexcept>

namespace tdc {
namespace meta {
namespace ast {

constexpr Acceptor sym_whitespace = Acceptor(" \t\n\r");

constexpr Acceptor sym_letter = Acceptor(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
constexpr Acceptor sym_digit = Acceptor("0123456789");

constexpr Acceptor sym_name_special = Acceptor("_");
constexpr UnionAcceptor sym_name = UnionAcceptor({
    sym_letter, sym_digit, sym_name_special});

class ParseError : public std::runtime_error {
public:
    inline ParseError(const std::string& err, size_t pos)
        : std::runtime_error(err + " (pos: " + std::to_string(pos+1) + ")") {
    }
};

/// \brief Provides functionality to parse a string into an AST (\ref ASTNode).
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

    inline std::shared_ptr<Primitive> parse_string_literal() {
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
        return std::make_shared<Primitive>(
            m_str.substr(start, m_cursor - 1 - start));
    }

    inline std::shared_ptr<Primitive> parse_numeric_literal() {
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
        return std::make_shared<Primitive>(
            m_str.substr(start, m_cursor - start));
    }

    inline std::shared_ptr<List> parse_list() {
        auto list = std::make_shared<List>();

        if(peek() != '[') throw ParseError("unexpected list opening", m_cursor);

        ++m_cursor; // opening bracket
        parse_whitespace();
        while(peek() != ']') {
            // parse values separated by commas
            auto v = parse_value();
            if(!v) throw ParseError("unexpected end of input", m_cursor);

            list->add_value(v);

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

    inline void parse_param_list(std::shared_ptr<Node> node) {
        if(peek() == '(') {
            // param list
            ++m_cursor; // opening paranthesis
            parse_whitespace();

            while(peek() != ')') {
                // attempt to parse a name first
                auto name = parse_name();
                if(name.empty()) {
                    // treat as nameless value
                    auto v = parse_value();
                    if(!v) {
                        throw ParseError("unexpected end of input", m_cursor);
                    }
                    node->add_param(Param(v));
                } else {
                    parse_whitespace();

                    // peek next character and decide what to do
                    auto c = peek();
                    if(c == '=') {
                        ++m_cursor;
                        parse_whitespace();

                        // parameter
                        auto v = parse_value();
                        if(!v) {
                            throw ParseError("unexpected end of input", m_cursor);
                        }
                        node->add_param(Param(name, v));
                    } else {
                        // treat as a nameless sub node
                        auto sub = std::make_shared<Node>(name);
                        parse_param_list(sub); // recursion!
                        node->add_param(Param(sub));
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

    inline std::shared_ptr<Node> parse_node() {
        // attempt to parse a name
        auto name = parse_name();
        if(name.empty()) {
            return std::shared_ptr<Node>(); // null
        } else {
            auto node = std::make_shared<Node>(name);
            parse_whitespace();
            parse_param_list(node);
            return node;
        }
    }

    inline std::shared_ptr<Value> parse_value() {
        parse_whitespace();

        // peek next character and decide what to do
        auto c = peek();
        if(c == 0) {
            // end
            return std::shared_ptr<Value>();
        } else if(c == '\'' || c == '\"') {
            // string literal, enclosed by quotes or double quotes
            return parse_string_literal();
        } else if(c == '+' || c == '-' || sym_digit.accept(c)) {
            // numeric literal
            return parse_numeric_literal();
        } else if(c == '[') {
            // a list of values
            return parse_list();
        } else {
            // attempt to parse a node
            auto node = parse_node();
            if(!node) {
                throw ParseError("syntax error", m_cursor);
            } else {
                return node;
            }
        }
    }

public:
    inline static std::shared_ptr<Value> parse(const std::string& str) {
        Parser p(str);
        return p.parse_value();
    }
};

}}} //ns
