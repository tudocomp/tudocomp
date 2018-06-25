#pragma once

#include <tudocomp/util/algorithm_parser/ASTDef.hpp>

#include <stdexcept>

namespace tdc {
    /// \cond INTERNAL
    namespace ast {
        class ParseError: public std::runtime_error {
        public:
            inline ParseError(const std::string& cause, size_t pos, std::string input):
                std::runtime_error(cause
                    + ", found "
                    + input.substr(pos)) {}
        };

        // A simple recursive parser
        class Parser {
            View m_text;
            size_t m_cursor;

            inline void error(const std::string& cause) {
                throw ParseError(cause, m_cursor, m_text);
            }
            inline void error_ident() {
                error("Expected an identifier");
            }

            typedef bool (*acceptor)(char c);

            template<char acc>
            static inline bool accept_char(char c) {
                return (c == acc);
            }

            static inline bool accept_numeric(char c) {
                //yes, very lenient
                return (c == '-') || (c == '.') || (c >= '0' && c <= '9');
            }

            static inline bool accept_non_numeric(char c) {
                return !accept_numeric(c);
            }
        public:
            inline Parser(View text): m_text(text), m_cursor(0) {}

            inline bool has_next();
            inline Value parse_value(View already_parsed_ident = View(""));
            inline Value parse_single_value(View already_parsed_ident = View(""));
            inline Arg parse_arg();
            inline View parse_ident();
            inline void parse_whitespace();
            inline bool parse_char(acceptor accept);
            inline bool peek_char(acceptor accept);
            inline std::string parse_string(
                acceptor delim, const std::string& help, bool enclose = true);
            inline bool parse_keyword(View keyword);
            inline View expect_ident();
        };

        inline bool Parser::has_next() {
            return m_cursor < m_text.size();
        }

        inline Value Parser::parse_single_value(View already_parsed_ident) {
            parse_whitespace();

            if (already_parsed_ident.size() == 0) {
                if (peek_char(accept_char<'"'>)) {
                    return Value(parse_string(accept_char<'"'>, "\""));
                }
                if (peek_char(accept_char<'\''>)) {
                    return Value(parse_string(accept_char<'\''>, "'"));
                }
                if (peek_char(accept_numeric)) {
                    return Value(parse_string(accept_non_numeric, "non-numeric", false));
                }
            }

            View value_name("");

            if (already_parsed_ident.size() > 0) {
                value_name = already_parsed_ident;
            } else {
                value_name = expect_ident();
            }

            if (value_name == "true"
                || value_name == "false") {
                return Value(value_name);
            }

            std::vector<Arg> args;
            bool first = true;
            parse_whitespace();
            if (parse_char(accept_char<'('>)) {
                while(true) {
                    parse_whitespace();
                    if (parse_char(accept_char<')'>)) {
                        break;
                    } else if (first || parse_char(accept_char<','>)) {
                        if (parse_char(accept_char<')'>)) {
                            // allow trailling commas
                            break;
                        }
                        first = false;
                        args.push_back(parse_arg());
                    } else {
                        error("Expected ) or ,");
                    }
                }
            }

            return Value(value_name, std::move(args));

        }

        inline Value Parser::parse_value(View already_parsed_ident) {
            Value val1 = parse_single_value(already_parsed_ident);
            while (parse_char(accept_char<':'>)) {
                Value val2 = parse_single_value();
                val1 = Value("chain", {
                    Arg(std::move(val1)),
                    Arg(std::move(val2)),
                });
            }
            return val1;
        }

        inline Arg Parser::parse_arg() {
            //return Arg(Value("test"));

            auto ident = parse_ident();

            bool has_type = false;
            bool is_static = false;
            View type_ident("");

            bool has_keyword = false;
            View keyword_ident("");

            if (ident.size() > 0 && parse_char(accept_char<'='>)) {
                keyword_ident = ident;
                ident.clear();
                has_keyword = true;
            }

            auto value = parse_value(ident);

            if (has_keyword && has_type) {
                return Arg(keyword_ident,
                           is_static,
                           type_ident,
                           std::move(value));
            } else if (!has_keyword && has_type) {
                return Arg(is_static,
                           type_ident,
                           std::move(value));
            } else if (has_keyword && !has_type) {
                return Arg(keyword_ident,
                           std::move(value));
            } else {
                return Arg(std::move(value));
            }
        }
        inline View Parser::parse_ident() {
            parse_whitespace();
            size_t ident_start = m_cursor;
            if (m_cursor < m_text.size()) {
                auto c = m_text[m_cursor];
                if (c == '_'
                    || (c >= 'a' && c <= 'z')
                    || (c >= 'A' && c <= 'Z')
                ) {
                    // char is valid in an IDENT
                    m_cursor++;
                } else {
                    return m_text.slice(ident_start, m_cursor);
                }
            }
            for (; m_cursor < m_text.size(); m_cursor++) {
                auto c = m_text[m_cursor];
                if (c == '_'
                    || (c >= 'a' && c <= 'z')
                    || (c >= 'A' && c <= 'Z')
                    || (c >= '0' && c <= '9')
                ) {
                    // char is valid in an IDENT
                } else {
                    break;
                }
            }
            return m_text.slice(ident_start, m_cursor);
        }
        inline void Parser::parse_whitespace() {
            if (!has_next()) {
                return;
            }
            while (m_text[m_cursor] == ' '
                || m_text[m_cursor] == '\n'
                || m_text[m_cursor] == '\r'
                || m_text[m_cursor] == '\t')
            {
                m_cursor++;
            }
        }
        inline bool Parser::parse_char(acceptor accept) {
            bool r = peek_char(accept);
            if (r) {
                m_cursor++;
            }
            return r;
        }
        inline bool Parser::peek_char(acceptor accept) {
            if (!has_next()) {
                return false;
            }
            parse_whitespace();
            return accept(m_text[m_cursor]);
        }
        inline std::string Parser::parse_string(
            acceptor delim, const std::string& help, bool enclose) {

            size_t start;
            size_t end;
            if (!enclose || parse_char(delim)) {
                start = m_cursor;
                end = start - 1;
                while(has_next()) {
                    if(enclose) {
                        if (parse_char(delim)) {
                            end = m_cursor - 1;
                            break;
                        }
                    } else {
                        if(peek_char(delim)) {
                            end = m_cursor;
                            break;
                        }
                    }
                    m_cursor++;
                }
                if (end >= start) {
                    return m_text.slice(start, end);
                }
            }
            error(std::string("Expected ") + help);
            return "";
        }
        inline bool Parser::parse_keyword(View keyword) {
            parse_whitespace();
            if (m_text.slice(m_cursor).starts_with(keyword)) {
                m_cursor += keyword.size();
                return true;
            }
            return false;
        }
        inline View Parser::expect_ident() {
            View ident = parse_ident();
            if (ident.size() == 0) {
                error_ident();
            }
            return ident;
        }
    }
    /// \endcond
}
