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

#include <tudostat/Stat.hpp>

using tudostat::Stat;

namespace tudocomp {

//DIY lexical cast
template<typename T> T lexical_cast(const std::string& s) {
    T val;
    std::stringstream(s) >> val;
    return val;
}

/// Local environment for a compression/encoding/decompression call.
///
/// Gives access to a statistic logger, and to environment
/// options that can be used to modify the default behavior of an algorithm.
class Env {
    std::map<std::string, const std::string> options;
    mutable std::set<std::string> known_options;

    std::stack<Stat> m_stat_stack;
    Stat m_stat_root;

public:
    inline Env() : m_stat_root(Stat("Root")) {}

    inline Env(std::map<std::string, const std::string> options_)
        : options(options_), m_stat_root(Stat("Root")) {
    }

    /// Returns a copy of the backing map.
    inline std::map<std::string, const std::string> get_options() const {
        return options;
    }

    /// Returns the set of options that got actually asked for by the algorithms
    inline std::set<std::string> get_known_options() const {
        return known_options;
    }

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
            return lexical_cast<T>(options.at(name));
        } else {
            return default_value;
        }
    };

    /// Log an error and end the current operation
    inline void error(const std::string& msg) {
        throw std::runtime_error(msg);
    }

    /// Returns a reference to the root statistics phase.
    inline Stat& stat_root() {
        return m_stat_root;
    }

    /// Begins a new statistics phase
    inline void stat_begin(const std::string& name) {
        m_stat_stack.push(Stat(name));
        Stat& stat = m_stat_stack.top();
        stat.begin();
    }

    /// Returns a reference to the current statistics phase.
    /// This reference is valid only until the phase is ended.
    inline Stat& stat_current() {
        return m_stat_stack.top();
    }

    /// Ends the current statistics phase.
    inline void stat_end() {
        assert(!m_stat_stack.empty());

        Stat& stat_ref = m_stat_stack.top();
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
};

}
#endif
