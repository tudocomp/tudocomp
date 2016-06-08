#ifndef _INCLUDED_ENV_HPP
#define _INCLUDED_ENV_HPP

#include <glog/logging.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>

#include <tudocomp/util.h>

namespace tudocomp {

//DIY lexical cast
template<typename T> T lexical_cast(const std::string& s) {
    T val;
    std::stringstream(s) >> val;
    return val;
}

class OptionValue;
class AlgorithmValue {
public:
    using ArgumentMap = std::map<std::string, OptionValue>;
    using StatMap = std::map<std::string, std::string>;
private:
    std::string m_name;
    ArgumentMap m_arguments;
    StatMap m_stats;
    friend class OptionValue;
public:
    inline AlgorithmValue(std::string&& name, ArgumentMap&& arguments):
        m_name(std::move(name)), m_arguments(std::move(arguments)) {}

    inline const std::string& name() const {
        return m_name;
    }
    inline ArgumentMap& arguments() {
        return m_arguments;
    }
    inline StatMap& statistics() {
        return m_stats;
    }
    /// Log a statistic.
    ///
    /// Statistics will be gathered at a central location, and
    /// can be used to judge the behavior and performance of an
    /// implementation.
    ///
    /// \param name The name of the statistic.
    /// \param value The value of the statistic as a string.
    template<class T>
    inline void log_stat(const std::string& name, const T value) {
        std::stringstream s;
        s << value;
        m_stats.emplace(name, s.str());
    };
    inline std::string statistics_to_string(const std::string& prefix = "", bool is_first = true);
};

class OptionValue {
    bool m_is_value;
    AlgorithmValue m_value_or_algorithm;
    friend class AlgorithmValue;
public:
    inline OptionValue(): OptionValue("") {}
    inline OptionValue(std::string&& value):
        m_is_value(true),
        m_value_or_algorithm(AlgorithmValue(std::move(value), {})) {}
    inline OptionValue(AlgorithmValue&& algorithm):
        m_is_value(false),
        m_value_or_algorithm(std::move(algorithm)) {}

    inline bool value_is_algorithm() {
        return !m_is_value;
    }
    inline bool has_value() const {
        return m_value_or_algorithm.m_name != "";
    }
    inline const std::string& value_as_string() const {
        CHECK(m_is_value);
        return m_value_or_algorithm.m_name;
    }
    inline AlgorithmValue& value_as_algorithm() {
        CHECK(!m_is_value);
        return m_value_or_algorithm;
    }
    inline uint64_t value_as_integer() const {
        return lexical_cast<uint64_t>(value_as_string());
    }
};

inline std::string AlgorithmValue::statistics_to_string(const std::string& prefix, bool is_first) {
    std::stringstream ss;
    std::string new_prefix;
    if (is_first) {
        new_prefix =
            std::string(prefix)
            + name();
    } else {
        new_prefix =
            std::string(prefix)
            + "."
            + name();
    }

    for (auto& s : statistics()) {
        ss << new_prefix << "." << s.first << " = " << s.second << "\n";
    }
    for (auto& a : arguments()) {
        if (!a.second.value_is_algorithm()) {
            continue;
        }
        auto& t = a.second.value_as_algorithm();
        ss << t.statistics_to_string(new_prefix, false);
    }
    return ss.str();
}

class EnvRoot {
    std::unique_ptr<AlgorithmValue> m_algo_value;
public:
    inline EnvRoot() {}

    inline EnvRoot(AlgorithmValue&& algo_value):
        m_algo_value(std::make_unique<AlgorithmValue>(std::move(algo_value))) {}

    inline AlgorithmValue& algo_value() {
        return *m_algo_value;
    }
};

/// Local environment for a compression/encoding/decompression call.
///
/// Gives access to a statistic logger, and to environment
/// options that can be used to modify the default behavior of an algorithm.
class Env {
    std::shared_ptr<EnvRoot> m_root;
    AlgorithmValue* m_node;

    inline Env() = delete;

public:
    inline Env(std::shared_ptr<EnvRoot> root, AlgorithmValue& node):
        m_root(root), m_node(&node) {}

    /// Log an error and end the current operation
    inline void error(const std::string& msg) {
        throw std::runtime_error(msg);
    }

    inline AlgorithmValue& algo() {
        return *m_node;
    }

    inline Env env_for_option(const std::string& option) {
        auto& a = algo().arguments()[option].value_as_algorithm();

        return Env(m_root, a);
    }

    inline std::shared_ptr<EnvRoot>& root() {
        return m_root;
    }
};

}
#endif
