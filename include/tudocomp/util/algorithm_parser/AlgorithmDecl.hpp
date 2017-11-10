#pragma once

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/util/algorithm_parser/ASTDef.hpp>

namespace tdc {
    /// \cond INTERNAL
    namespace decl {
        /*
            Representation of the declaration in the Registry.
            It cares about the exact types and signatures of the invokations,
            and about the documentation for the help printout
        */
        class Algorithm;
        class Arg;
        class Algorithm {
            std::string m_name;
            std::vector<Arg> m_arguments;
            std::string m_doc;
            ds::InputRestrictionsAndFlags m_ds_flags;

        public:

            inline Algorithm(std::string&& name,
                             std::vector<Arg>&& args,
                             std::string&& doc,
                             ds::InputRestrictionsAndFlags flags):
                m_name(std::move(name)),
                m_arguments(std::move(args)),
                m_doc(std::move(doc)),
                m_ds_flags(flags) {}

            inline const std::string& name() const {
                return m_name;
            }

            inline const std::vector<Arg>& arguments() const {
                return m_arguments;
            }
            inline std::vector<Arg>& arguments() {
                return m_arguments;
            }

            inline const std::string& doc() const {
                return m_doc;
            }

            inline ds::InputRestrictionsAndFlags textds_flags() {
                return m_ds_flags;
            }

            inline std::string to_string(bool omit_type = false) const;

            friend inline bool operator==(const Algorithm &lhs, const Algorithm &rhs);
        };
        class Arg {
            std::string m_name;
            bool m_is_static;
            std::string m_type;
            bool m_has_default;
            ast::Value m_default;

        public:

            inline Arg(std::string&& name,
                       bool is_static,
                       std::string&& type):
                m_name(std::move(name)),
                m_is_static(is_static),
                m_type(std::move(type)),
                m_has_default(false) {}
            inline Arg(std::string&& name,
                       bool is_static,
                       std::string&& type,
                       ast::Value&& default_value):
                m_name(std::move(name)),
                m_is_static(is_static),
                m_type(std::move(type)),
                m_has_default(true),
                m_default(std::move(default_value)) {}

            inline const std::string& name() const {
                return m_name;
            }
            inline bool is_static() const {
                return m_is_static;
            }
            inline const std::string& type() const {
                return m_type;
            }
            inline bool has_default() const {
                return m_has_default;
            }

            inline const ast::Value& default_value() const {
                return m_default;
            }
            inline ast::Value& default_value() {
                return m_default;
            }

            inline std::string to_string(bool omit_type = false) const;

            friend inline bool operator==(const Arg &lhs, const Arg &rhs);
        };

        inline std::ostream& operator<<(std::ostream& os,
                                        const Algorithm& x) {
            os << x.to_string();
            return os;
        }
        inline std::ostream& operator<<(std::ostream& os,
                                        const Arg& x) {
            os << x.to_string();
            return os;
        }

        inline std::string Algorithm::to_string(bool omit_type) const {
            std::stringstream ss;
            ss << name();
            if (arguments().size() > 0) {
                ss << "(";
                bool first = true;
                for (auto& a: arguments()) {
                    if (!first) {
                        ss << ", ";
                    }
                    ss << a.to_string(omit_type);
                    first = false;
                }
                ss << ")";
            }
            return ss.str();
        }

        inline bool operator==(const Arg &lhs, const Arg &rhs);
        inline bool operator==(const Algorithm &lhs, const Algorithm &rhs);

        inline bool operator!=(const Arg &lhs, const Arg &rhs) {
            return !(lhs == rhs);
        }
        inline bool operator!=(const Algorithm &lhs, const Algorithm &rhs) {
            return !(lhs == rhs);
        }

        inline bool operator==(const Arg &lhs, const Arg &rhs) {
            if (lhs.m_name != rhs.m_name) return false;
            if (lhs.m_is_static != rhs.m_is_static) return false;
            if (lhs.m_type != rhs.m_type) return false;
            if (lhs.m_has_default != rhs.m_has_default) return false;
            if (lhs.m_default != rhs.m_default) return false;
            return true;
        }

        inline bool operator==(const Algorithm &lhs, const Algorithm &rhs) {
            if (lhs.m_name != rhs.m_name) return false;
            if (lhs.m_arguments != rhs.m_arguments) return false;
            if (lhs.m_doc != rhs.m_doc) return false;
            return true;
        }

        inline std::string Arg::to_string(bool omit_type) const {
            std::stringstream ss;
            ss << name();
            if (!omit_type) {
                ss << ": ";
                if (is_static()) {
                    ss << "static ";
                }
                ss << type();
            }
            if (has_default()) {
                ss << " = " << default_value();
            }
            return ss.str();
        }
    }
    /// \endcond
}
