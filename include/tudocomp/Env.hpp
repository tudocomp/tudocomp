#ifndef _INCLUDED_ENV_HPP
#define _INCLUDED_ENV_HPP

#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>

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
    inline const ArgumentMap& arguments() const {
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

    inline bool has_value() const {
        return m_value_or_algorithm.m_name != "";
    }
    inline const std::string& value_as_string() const {
        CHECK(m_is_value);
        return m_value_or_algorithm.m_name;
    }
    inline const AlgorithmValue& value_as_algorithm() const {
        CHECK(!m_is_value);
        return m_value_or_algorithm;
    }
    inline uint64_t value_as_integer() const {
        return lexical_cast<uint64_t>(value_as_string());
    }
    inline AlgorithmValue::ArgumentMap into_algorithm_options() {
        value_as_algorithm();
        auto tmp = std::move(m_value_or_algorithm);
        return std::move(tmp.m_arguments);
    }
};

/*

class GlobalEnv {

};

class LocalEnv {
    AlgorithmValue::ArgumentMap m_options;
    GlobalEnv* m_global_env;
public:
    inline LocalEnv() {}

    //inline GlobalEnv& global_env() {
    //    return *m_global_env;
    //}

    //const AlgorithmValue::ArgumentMap
};

*/

/// Local environment for a compression/encoding/decompression call.
///
/// Gives access to a statistic logger, and to environment
/// options that can be used to modify the default behavior of an algorithm.
class Env {
    AlgorithmValue::ArgumentMap m_options;
    //GlobalEnv* m_global_env;

public:
    inline Env() {}
    inline Env(AlgorithmValue::ArgumentMap&& options):
        m_options(options) {}

    /// Returns a copy of the backing map.
    inline std::map<std::string, const std::string> get_options() const {
        return {};
    }

    /// Returns a copy of the backing map.
    inline std::map<std::string, const std::string> get_stats() const {
        return {};
    }

    /// Returns the set of options that got actually asked for by the algorithms
    inline std::set<std::string> get_known_options() const {
        return {};
    }

    /// Log a statistic.
    ///
    /// Statistics will be gathered at a central location, and
    /// can be used to judge the behavior and performance of an
    /// implementation.
    ///
    /// \param name The name of the statistic. Should be a unique identifier
    ///             with `.`-separated segments to allow easier grouping of
    ///             the gathered values. For example:
    ///             `"my_compressor.phase1.alphabet_size"`.
    /// \param value The value of the statistic as a string.
    template<class T>
    inline void log_stat(const std::string& name, const T value) {
        //std::stringstream s;
        //s << value;
        //stats.emplace(name, s.str());
    };

    /// Returns whether a option has been set.
    inline bool has_option(const std::string& name) const {
        //known_options.insert(name);
        //return options.count(name) > 0;
        return false;
    };

    /// Returns a option as its raw string value, or the empty string if
    /// it is not set..
    ///
    /// \param name The name of the option. Should be a unique identifier
    ///             with `.`-separated segments to allow easier grouping.
    ///             For example:
    ///             `"my_compressor.xyz_threshold"`.
    /// \param default_value The default value to use if the option is not set.
    ///                      Defaults to the empty string.
    inline const std::string& option(const std::string& name, const std::string& default_value = "") const {
        //known_options.insert(name);
        //if (has_option(name)) {
        //    return options.at(name);
        //} else
        {
            return default_value;
        }
    };

    /// Returns a option by casting its raw string value
    /// to the template type `T`.
    /// If the option does not exists, it returns the `default_value` argument.
    ///
    /// Example:
    /// ```
    /// int threshold = env.option_as<int>("my_compressor.xyz_threshold", 3);
    /// ```
    ///
    /// \param name The name of the option. Should be a unique identifier
    ///             with `.`-separated segments to allow easier grouping.
    ///             For example:
    ///             `"my_compressor.xyz_threshold"`.
    /// \param default_value The default value to use if the option is not set.
    ///                      Defaults to the default-constructed value of `T`.
    template<class T>
    T option_as(const std::string& name, T default_value = T()) const {
        //known_options.insert(name);
        //if (has_option(name)) {
        //    return lexical_cast<T>(options.at(name));
        //} else
        {
            return default_value;
        }
    };

    /// Log an error and end the current operation
    inline void error(const std::string& msg) {
        throw std::runtime_error(msg);
    }
};

}
#endif
