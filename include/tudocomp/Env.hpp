#ifndef _INCLUDED_ENV_HPP
#define _INCLUDED_ENV_HPP

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#include <glog/logging.h>

#include <tudostat/Stat.hpp>

using tudostat::Stat;

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

class EnvRoot {
private:
    std::unique_ptr<AlgorithmValue> m_algo_value;

    std::stack<Stat> m_stat_stack;
    Stat m_stat_root;

public:
    inline EnvRoot() {
        begin_stat_phase("root");
    }

    inline EnvRoot(AlgorithmValue&& algo_value):
        m_algo_value(std::make_unique<AlgorithmValue>(std::move(algo_value)))  {

        begin_stat_phase("root");
    }

    ~EnvRoot() {
        finish_stats();
    }

    inline AlgorithmValue& algo_value() {
        return *m_algo_value;
    }

    /// Returns a reference to the current statistics phase.
    /// This reference is valid only until the phase is ended.
    inline Stat& stat_current() {
        return m_stat_stack.empty() ? m_stat_root : m_stat_stack.top();
    }

    /// Begins a new statistics phase
    inline void begin_stat_phase(const std::string& name) {
        DLOG(INFO) << "begin phase \"" << name << "\"";

        m_stat_stack.push(Stat(name));
        Stat& stat = m_stat_stack.top();
        stat.begin();
    }

    /// Ends the current statistics phase.
    inline void end_stat_phase() {
        assert(!m_stat_stack.empty());

        Stat& stat_ref = m_stat_stack.top();
        DLOG(INFO) << "end phase \"" << stat_ref.title() << "\"";

        stat_ref.end();

        Stat stat = stat_ref; //copy
        m_stat_stack.pop();

        if(!m_stat_stack.empty()) {
            Stat& parent = m_stat_stack.top();
            parent.add_sub(stat);
        } else {
            m_stat_root = stat;
        }
    }

    /// Ends all current statistics phases and returns the root.
    inline Stat& finish_stats() {
        while(!m_stat_stack.empty()) {
            end_stat_phase();
        }

        return m_stat_root;
    }

    /// Resets all statistics and restarts with a new root.
    inline void restart_stats(const std::string& root_name) {
        finish_stats();
        begin_stat_phase(root_name);
    }

    /// Logs a statistic
    template<class T>
    inline void log_stat(const std::string& name, const T value) {
        DLOG(INFO) << "stat: " << name << " = " << value;
        stat_current().add_stat(name, value);
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

    inline AlgorithmValue& algo() {
        return *m_node;
    }

public:
    inline Env(std::shared_ptr<EnvRoot> root, AlgorithmValue& node):
        m_root(root), m_node(&node) {}

    inline std::shared_ptr<EnvRoot>& root() {
        return m_root;
    }

    /// Log an error and end the current operation
    inline void error(const std::string& msg) {
        throw std::runtime_error(msg);
    }

    /// Create the environment for a sub algorithm
    /// option.
    inline Env env_for_option(const std::string& option) {
        CHECK(algo().arguments().count(option) > 0);
        auto& a = algo().arguments()[option].value_as_algorithm();

        return Env(m_root, a);
    }

    /// Get an option of this algorithm
    inline OptionValue& option(const std::string& option) {
        return algo().arguments()[option];
    }

    /// Begins a new statistics phase
    inline void begin_stat_phase(const std::string& name) {
        m_root->begin_stat_phase(name); //delegate
    }

    /// Ends the current statistics phase.
    inline void end_stat_phase() {
        m_root->end_stat_phase(); //delegate
    }

    /// Ends all current statistics phases and returns the root.
    inline Stat& finish_stats() {
        return m_root->finish_stats(); //delegate
    }

    /// Resets all statistics and restarts with a new root.
    inline void restart_stats(const std::string& root_name) {
        m_root->restart_stats(root_name); //delegate
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
        m_root->log_stat(name, value);
    };
};

}
#endif
