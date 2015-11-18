#ifndef TUDOCOMPENV_H
#define TUDOCOMPENV_H

#include <map>
#include <vector>
#include <string>
#include <sstream>

#include <boost/utility/string_ref.hpp>
#include <boost/lexical_cast.hpp>

namespace tudocomp {

/// Local environment for a compression/encoding/decompression call.
///
/// Gives access to a statistic logger, and to environment
/// options that can be used to modify the default behavior of an algorithm.
class Env {
    std::map<std::string, std::string> options;
    std::map<std::string, std::string> stats;
    boost::string_ref algorithm_id;
public:
    inline Env() {}
    inline Env(std::map<std::string, std::string> options_,
               std::map<std::string, std::string> stats_,
               boost::string_ref algorithm_id_):
        options(options_), stats(stats_), algorithm_id(algorithm_id_) {}

    /// Returns a copy of the backing map.
    inline std::map<std::string, std::string> get_options() {
        return options;
    }

    /// Returns a copy of the backing map.
    inline std::map<std::string, std::string> get_stats() {
        return stats;
    }

    inline boost::string_ref pop_front_algorithm_id() {
        auto idx = algorithm_id.find('.');
        boost::string_ref r = algorithm_id.substr(0, idx);
        algorithm_id.remove_prefix(idx);
        return r;
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
    inline void log_stat(std::string name, T value) {
        std::stringstream s;
        s << value;
        stats[name] = s.str();
    };

    /// Returns whether a option has been set.
    inline bool has_option(const std::string& name) {
        return options.count(name) > 0;
    };

    /// Returns a option as its raw string value, or the empty string if
    /// it is not set..
    ///
    /// \param name The name of the option. Should be a unique identifier
    ///             with `.`-separated segments to allow easier grouping.
    ///             For example:
    ///             `"my_compressor.xyz_threshold"`.
    inline std::string option(std::string name) {
        if (has_option(name)) {
            return options[name];
        } else {
            return "";
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
    T option_as(std::string name, T default_value = T()) {
        if (has_option(name)) {
            return boost::lexical_cast<T>(options[name]);
        } else {
            return default_value;
        }
    };
};

}
#endif
