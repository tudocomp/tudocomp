#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <istream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <glog/logging.h>

#include <tudocomp/def.hpp>
#include <tudocomp/Stat.hpp>

namespace tdc {

class Registry;
inline std::unique_ptr<Registry> make_ptr_copy_of_registry(const Registry& registry);

class EnvRoot;
class Env;

class StatGuard {
    friend class EnvRoot;
    friend class Env;

    EnvRoot* m_env_root;
    bool m_is_done = false;
    inline StatGuard(EnvRoot& root):
        m_env_root(&root) {}
public:
    void end();
    ~StatGuard();
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

    inline ~EnvRoot() {
        finish_stats();
    }

    inline AlgorithmValue& algo_value();

    /// Returns a reference to the current statistics phase.
    /// This reference is valid only until the phase is ended.
    inline Stat& stat_current();

    /// Begins a new statistics phase
    inline void begin_stat_phase(const std::string& name);

    /// Ends the current statistics phase.
    inline void end_stat_phase();

    /// Begins a new statistics phase
    ///
    /// The phase ends if the returned guard gets destroyed.
    inline StatGuard stat_phase(const std::string& name);

    /// Ends all current statistics phases and returns the root.
    inline Stat& finish_stats();

    /// Resets all statistics and restarts with a new root.
    inline void restart_stats(const std::string& root_name);

    /// Logs a statistic
    template<class T>
    inline void log_stat(const std::string& name, const T& value);
};

inline void StatGuard::end() {
    if (m_is_done) return;

    m_env_root->end_stat_phase();

    m_is_done = true;
}
inline StatGuard::~StatGuard() {
    end();
}

/// Local environment for a compression/encoding/decompression call.
///
/// Gives access to a statistic logger, and to environment
/// options that can be used to modify the default behavior of an algorithm.
class Env {
    std::shared_ptr<EnvRoot> m_root;
    AlgorithmValue m_node;
    std::unique_ptr<Registry> m_registry;

    inline const AlgorithmValue& algo() const;

public:
    inline Env() = delete;
    inline Env(const Env& other) = delete;
    inline Env(Env&& other);
    inline Env(std::shared_ptr<EnvRoot> root,
               const AlgorithmValue& node,
               const Registry& registry);
    inline ~Env();

    inline std::shared_ptr<EnvRoot>& root();

    inline const Registry& registry() const;

    /// Log an error and end the current operation
    inline void error(const std::string& msg);

    /// Create the environment for a sub algorithm
    /// option.
    inline Env env_for_option(const std::string& option);

    /// Get an option of this algorithm
    inline const OptionValue& option(const std::string& option) const;

    /// Begins a new statistics phase
    inline void begin_stat_phase(const std::string& name);

    /// Ends the current statistics phase.
    inline void end_stat_phase();

    /// Begins a new statistics phase
    ///
    /// The phase ends if the returned guard gets destroyed.
    inline StatGuard stat_phase(const std::string& name);

    /// Ends all current statistics phases and returns the root.
    inline Stat& finish_stats();

    /// Resets all statistics and restarts with a new root.
    inline void restart_stats(const std::string& root_name);

    /// Log a statistic.
    ///
    /// Statistics will be gathered at a central location, and
    /// can be used to judge the behavior and performance of an
    /// implementation.
    ///
    /// \param name The name of the statistic.
    /// \param value The value of the statistic as a string.
    template<class T>
    inline void log_stat(const std::string& name, const T& value);
};

}
