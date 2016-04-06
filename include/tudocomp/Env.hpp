#ifndef _INCLUDED_ENV_HPP
#define _INCLUDED_ENV_HPP

#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>

#include <boost/utility/string_ref.hpp>
#include <boost/lexical_cast.hpp>

namespace tudocomp {

/// Local environment for a compression/encoding/decompression call.
///
/// Gives access to a statistic logger, and to environment
/// options that can be used to modify the default behavior of an algorithm.
class Env {
    std::map<std::string, const std::string> options;
    std::map<std::string, const std::string> stats;
    mutable std::set<std::string> known_options;

public:
    inline Env() {}
    inline Env(std::map<std::string, const std::string> options_,
               std::map<std::string, const std::string> stats_):
        options(options_), stats(stats_) {}

    /// Returns a copy of the backing map.
    inline std::map<std::string, const std::string> get_options() const {
        return options;
    }

    /// Returns a copy of the backing map.
    inline std::map<std::string, const std::string> get_stats() const {
        return stats;
    }

    /// Returns the set of options that got actually asked for by the algorithms
    inline std::set<std::string> get_known_options() const {
        return known_options;
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
        std::stringstream s;
        s << value;
        stats.emplace(name, s.str());
    };

    /// Returns whether a option has been set.
    inline bool has_option(const std::string& name) const {
        known_options.insert(name);
        return options.count(name) > 0;
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
        known_options.insert(name);
        if (has_option(name)) {
            return options.at(name);
        } else {
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
        known_options.insert(name);
        if (has_option(name)) {
            return boost::lexical_cast<T>(options.at(name));
        } else {
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
