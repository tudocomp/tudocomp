#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <tudocomp/util/View.hpp>

namespace tdc {
    /// \cond INTERNAL
    namespace ast {
        class Value;
        class Arg;
        class Value {
            bool m_is_invokation;
            std::string m_invokation_name_or_value;
            std::vector<Arg> m_invokation_arguments;
        public:
            inline Value():
                m_is_invokation(false),
                m_invokation_name_or_value("") {}
            inline Value(std::string&& value):
                m_is_invokation(false),
                m_invokation_name_or_value(std::move(value)) {}
            inline Value(std::string&& name, std::vector<Arg>&& args):
                m_is_invokation(true),
                m_invokation_name_or_value(std::move(name)),
                m_invokation_arguments(std::move(args)) {}

            inline bool is_invokation() const {
                return m_is_invokation;
            }

            inline const std::string& invokation_name() const {
                CHECK(is_invokation());
                return m_invokation_name_or_value;
            }
            inline std::string& invokation_name() {
                CHECK(is_invokation());
                return m_invokation_name_or_value;
            }

            inline const std::vector<Arg>& invokation_arguments() const {
                CHECK(is_invokation());
                return m_invokation_arguments;
            }
            inline std::vector<Arg>& invokation_arguments() {
                CHECK(is_invokation());
                return m_invokation_arguments;
            }

            inline const std::string& string_value() const {
                CHECK(!is_invokation());
                return m_invokation_name_or_value;
            }
            inline std::string& string_value() {
                CHECK(!is_invokation());
                return m_invokation_name_or_value;
            }

            inline std::string to_string() const;

            inline bool is_empty() const {
                return m_invokation_name_or_value == "";
            }

            friend inline bool operator==(const Value &lhs, const Value &rhs);
        };
        class Arg {
            bool m_has_keyword;
            bool m_has_type;
            bool m_type_is_static;
            Value m_value;
            std::string m_keyword;
            std::string m_type;
        public:
            inline Arg(Value&& value):
                m_has_keyword(false),
                m_has_type(false),
                m_type_is_static(false),
                m_value(std::move(value)) {}
            inline Arg(View keyword, Value&& value):
                m_has_keyword(true),
                m_has_type(false),
                m_type_is_static(false),
                m_value(std::move(value)),
                m_keyword(keyword)
                {}
            inline Arg(View keyword, bool is_static, View type, Value&& value):
                m_has_keyword(true),
                m_has_type(true),
                m_type_is_static(is_static),
                m_value(std::move(value)),
                m_keyword(keyword),
                m_type(type)
                {}
            inline Arg(bool is_static, View type, Value&& value):
                m_has_keyword(false),
                m_has_type(true),
                m_type_is_static(is_static),
                m_value(std::move(value)),
                m_type(type)
                {}

            inline bool has_keyword() const {
                return m_has_keyword;
            }
            inline bool has_type() const {
                return m_has_type;
            }
            inline bool type_is_static() const {
                return m_type_is_static;
            }
            inline const std::string& keyword() const {
                return m_keyword;
            }
            inline std::string& keyword() {
                return m_keyword;
            }

            inline const std::string& type() const {
                return m_type;
            }
            inline std::string& type() {
                return m_type;
            }

            inline const Value& value() const {
                return m_value;
            }
            inline Value& value() {
                return m_value;
            }

            inline std::string to_string() const;

            friend inline bool operator==(const Arg &lhs, const Arg &rhs);
        };

        inline std::ostream& operator<<(std::ostream& os,
                                        const Value& x) {
            os << x.to_string();
            return os;
        }
        inline std::ostream& operator<<(std::ostream& os,
                                        const Arg& x) {
            os << x.to_string();
            return os;
        }

        inline bool operator==(const Arg &lhs, const Arg &rhs);
        inline bool operator==(const Value &lhs, const Value &rhs);

        inline bool operator!=(const Arg &lhs, const Arg &rhs) {
            return !(lhs == rhs);
        }
        inline bool operator!=(const Value &lhs, const Value &rhs) {
            return !(lhs == rhs);
        }

        inline bool operator==(const Arg &lhs, const Arg &rhs) {
            if (lhs.m_has_keyword != rhs.m_has_keyword) return false;
            if (lhs.m_has_type != rhs.m_has_type) return false;
            if (lhs.m_type_is_static != rhs.m_type_is_static) return false;
            if (lhs.m_keyword != rhs.m_keyword) return false;
            if (lhs.m_type != rhs.m_type) return false;
            if (lhs.m_value != rhs.m_value) return false;
            return true;
        }

        inline bool operator==(const Value &lhs, const Value &rhs) {
            if (lhs.m_is_invokation != rhs.m_is_invokation) return false;
            if (lhs.m_invokation_name_or_value != rhs.m_invokation_name_or_value) return false;
            if (lhs.m_invokation_arguments != rhs.m_invokation_arguments) return false;
            return true;
        }

        inline std::string Value::to_string() const {
            std::stringstream ss;
            if (is_invokation()) {
                ss << invokation_name();
                if (invokation_arguments().size() > 0) {
                    ss << "(";
                    bool first = true;
                    for (auto& a: invokation_arguments()) {
                        if (!first) {
                            ss << ", ";
                        }
                        ss << a;
                        first = false;
                    }
                    ss << ")";
                }
            } else {
                ss << "\"" << string_value() << "\"";
            }
            return ss.str();
        }

        inline std::string Arg::to_string() const {
            std::stringstream ss;
            if (has_keyword()) {
                ss << keyword();
            } else {
                ss << value();
            }
            if (has_type()) {
                ss << ": ";
                if (type_is_static()) {
                    ss << "static ";
                }
                ss << type();
            }
            if (has_keyword()) {
                ss << " = ";
                ss << value();
            }
            return ss.str();
        }
    }
    /// \endcond
}