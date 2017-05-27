#pragma once

#include <tudocomp/util/meta/ASTAcceptor.hpp>
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

    inline std::string parse_string_value() {
        // save enclosing character (e.g. quote or double quote)
        const char enclose = peek();

        // save starting position
        size_t start = ++m_cursor;

        // parse until enclosing character is found
        while(peek() != enclose) {
            if(peek() == 0) {
                throw ParseError("unexpected end of string", m_cursor);
            }
            ++m_cursor;
        }
        ++m_cursor; //enclose

        // yield substring
        return m_str.substr(start, m_cursor - 1 - start);
    }

    inline std::string parse_numeric_value() {
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
        return m_str.substr(start, m_cursor - start);
    }

    inline std::string parse_value() {
        if(peek() == '\'' || peek() == '\"') {
            return parse_string_value();
        } else if(peek() == '+' || peek() == '-' || sym_digit.accept(peek())) {
            return parse_numeric_value();
        } else {
            return "";
        }
    }

    inline void consume(size_t num) {
        m_cursor += num;
        parse_whitespace();
    }

    inline Node parse_node_remainder(std::string&& name) {
        parse_whitespace();

        // name already parsed
        Node node(name);

        if(peek() == '(') {
            consume(1); // opening paranthesis

            // parse child list
            while(peek() != ')') {
                std::string kn = parse_name();
                if(kn.empty()) {
                    // expect a keyless value
                    std::string value = parse_value();
                    if(value.empty()) {
                        throw ParseError(
                            "failed to parse either name or value", m_cursor);
                    } else {
                        node.add_child(Node("", value));
                    }
                } else {
                    parse_whitespace();

                    // kn may be a value key, OR the name of a keyless subnode
                    if(peek() == '=') {
                        consume(1); // assignment

                        // value key
                        std::string value = parse_value();
                        if(value.empty()) {
                            // expect keyed subnode
                            node.add_child(kn, parse_node());
                        } else {
                            // keyed value
                            node.add_child(kn, Node("", value));
                        }
                    } else if(peek() == '(') {
                        // keyless subnode
                        node.add_child(parse_node_remainder(std::move(kn)));
                    }
                }

                parse_whitespace();
                if(peek() == 0) {
                    throw ParseError("unexpected end of child list", m_cursor);
                } else if(peek() == ',') {
                    consume(1); // separator
                } else if(peek() != ')') {
                    throw ParseError(
                        "expected separator or closing paranthesis", m_cursor);
                }
            }
            consume(1); // closing paranthesis
        }

        return node;
    }

    inline Node parse_node() {
        parse_whitespace();

        // parse node's name
        std::string name = parse_name();
        if(name.empty()) {
            throw ParseError("node name must not be empty", m_cursor);
        }

        return parse_node_remainder(std::move(name));
    }

public:
    inline static Node parse(const std::string& str) {
        Parser p(str);
        auto node = p.parse_node();

        if(p.peek() != 0) {
            throw ParseError("unexpected character beyond root node", 0);
        }

        return node;
    }
};

}}} //ns
