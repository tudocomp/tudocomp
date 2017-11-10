#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace tdc {
    /// \cond INTERNAL
    namespace pattern {
        /*
            Representation of an algorithm consisting of only those
            options that are relevant for selecting a statically
            defined implementation
        */
        class Algorithm;
        class Arg;
        class Algorithm {
            std::string m_name;
            std::vector<Arg> m_arguments;
        public:
            inline Algorithm() {}
            inline Algorithm(std::string&& name, std::vector<Arg>&& args):
                m_name(std::move(name)),
                m_arguments(std::move(args)) {}
            inline Algorithm(const Algorithm& other):
                m_name(other.m_name),
                m_arguments(other.m_arguments) {}

            inline std::string& name() {
                return m_name;
            }
            inline const std::string& name() const {
                return m_name;
            }
            inline std::vector<Arg>& arguments() {
                return m_arguments;
            }
            inline const std::vector<Arg>& arguments() const {
                return m_arguments;
            }
            inline std::string to_string(bool omit_keyword = false) const;
        };
        class Arg {
            std::string m_name;
            Algorithm m_algorithm;
        public:
            inline Arg(std::string&& name, Algorithm&& alg):
                m_name(std::move(name)),
                m_algorithm(std::move(alg)) {}
            inline std::string& name() {
                return m_name;
            }
            inline const std::string& name() const {
                return m_name;
            }
            inline Algorithm& algorithm() {
                return m_algorithm;
            }
            inline const Algorithm& algorithm() const {
                return m_algorithm;
            }
            inline std::string to_string(bool omit_keyword = false) const;
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

        inline std::string Algorithm::to_string(bool omit_keyword) const {
            std::stringstream ss;
            ss << name();
            if (arguments().size() > 0) {
                ss << "(";
                bool first = true;
                for (auto& a: arguments()) {
                    if (!first) {
                        ss << ", ";
                    }
                    ss << a.to_string(omit_keyword);
                    first = false;
                }
                ss << ")";
            }
            return ss.str();
        }

        inline std::string Arg::to_string(bool omit_keyword) const {
            std::stringstream ss;
            if (omit_keyword) {
                ss << algorithm().to_string(omit_keyword);
            } else {
                ss << name() << " = " << algorithm().to_string(omit_keyword);
            }
            return ss.str();
        }

        inline bool operator==(const Algorithm &lhs, const Algorithm &rhs);
        inline bool operator<(const Algorithm &lhs, const Algorithm &rhs);
        inline bool operator==(const Arg &lhs, const Arg &rhs);
        inline bool operator<(const Arg &lhs, const Arg &rhs);

        inline bool operator!=(const Algorithm &lhs, const Algorithm &rhs) {
            return !(lhs == rhs);
        }
        inline bool operator!=(const Arg &lhs, const Arg &rhs) {
            return !(lhs == rhs);
        }

        inline bool operator==(const Algorithm &lhs, const Algorithm &rhs) {
            if (lhs.name() != rhs.name()) return false;
            if (lhs.arguments() != rhs.arguments()) return false;
            return true;
        }
        inline bool operator<(const Algorithm &lhs, const Algorithm &rhs) {
            if (lhs.name() != rhs.name()) return lhs.name() < rhs.name();
            if (lhs.arguments() != rhs.arguments()) return lhs.arguments() < rhs.arguments();
            return false;
        }
        inline bool operator==(const Arg &lhs, const Arg &rhs) {
            if (lhs.name() != rhs.name()) return false;
            if (lhs.algorithm() != rhs.algorithm()) return false;
            return true;
        }
        inline bool operator<(const Arg &lhs, const Arg &rhs) {
            if (lhs.name() != rhs.name()) return lhs.name() < rhs.name();
            if (lhs.algorithm() != rhs.algorithm()) return lhs.algorithm() < rhs.algorithm();
            return false;
        }
    }
    /// \endcond
}
