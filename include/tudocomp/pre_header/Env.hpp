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
#include <tudocomp/OptionValue.hpp>

namespace tdc {

class EnvRoot;
class Env;

class EnvRoot {
private:
    std::unique_ptr<AlgorithmValue> m_algo_value;

public:
    inline EnvRoot() {
    }

    inline EnvRoot(AlgorithmValue&& algo_value):
        m_algo_value(std::make_unique<AlgorithmValue>(std::move(algo_value)))  {
    }

    inline ~EnvRoot() {
    }

    inline AlgorithmValue& algo_value();
};

/// Local environment for a compression/encoding/decompression call.
///
/// Gives access to environment
/// options that can be used to modify the default behavior of an algorithm.
class Env {
    std::shared_ptr<EnvRoot> m_root;
    AlgorithmValue m_node;

    inline const AlgorithmValue& algo() const;

public:
    inline Env() = delete;
    inline Env(const Env& other) = delete;
    inline Env(Env&& other);
    inline Env(std::shared_ptr<EnvRoot> root,
               const AlgorithmValue& node);
    inline ~Env();

    inline const std::shared_ptr<EnvRoot>& root() const;

    /// Log an error and end the current operation
    inline void error(const std::string& msg) const;

    /// Create the environment for a sub algorithm
    /// option.
    inline Env env_for_option(const std::string& option) const;

    /// Get an option of this algorithm
    inline const OptionValue& option(const std::string& option) const;
};

}
