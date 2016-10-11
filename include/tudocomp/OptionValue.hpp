#ifndef _INCLUDED_OPTION_VALUE_HPP
#define _INCLUDED_OPTION_VALUE_HPP

#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#include <glog/logging.h>

#include <tudocomp/util.h>

namespace tdc {

// Forward declaration to be used by the parser
namespace pattern {
    class Algorithm;
}

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
    // Aways valid!
    std::unique_ptr<pattern::Algorithm> m_static_selection;
    friend class OptionValue;
public:
    inline AlgorithmValue(const AlgorithmValue& other):
        m_name(other.m_name),
        m_arguments(other.m_arguments)
    {
        if (other.m_static_selection != nullptr) {
            m_static_selection = std::make_unique<pattern::Algorithm>(
                *other.m_static_selection);
        }
    }

    inline AlgorithmValue(AlgorithmValue&& other):
        m_name(std::move(other.m_name)),
        m_arguments(std::move(other.m_arguments)),
        m_static_selection(std::move(other.m_static_selection)) {}

    inline AlgorithmValue(std::string&& name,
                          ArgumentMap&& arguments,
                          std::unique_ptr<pattern::Algorithm>&& static_selection):
        m_name(std::move(name)),
        m_arguments(std::move(arguments)),
        m_static_selection(std::move(static_selection)) {}

    inline const std::string& name() const {
        return m_name;
    }
    inline const ArgumentMap& arguments() const {
        return m_arguments;
    }
    inline const pattern::Algorithm& static_selection() const {
        CHECK(m_static_selection != nullptr);
        return *m_static_selection;
    }
    inline AlgorithmValue& operator=(AlgorithmValue&& other) {
        this->m_name = std::move(other.m_name);
        this->m_arguments = std::move(other.m_arguments);
        this->m_static_selection = std::move(other.m_static_selection);
        return *this;
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
        m_value_or_algorithm(AlgorithmValue(std::move(value), {}, std::make_unique<pattern::Algorithm>())) {}
    inline OptionValue(AlgorithmValue&& algorithm):
        m_is_value(false),
        m_value_or_algorithm(std::move(algorithm)) {}

    inline bool is_algorithm() const {
        return !m_is_value;
    }
    inline const AlgorithmValue& as_algorithm() const {
        CHECK(!m_is_value);
        return m_value_or_algorithm;
    }
    inline const std::string& as_string() const {
        CHECK(m_is_value);
        return m_value_or_algorithm.m_name;
    }
    inline uint64_t as_integer() const {
        return lexical_cast<uint64_t>(as_string());
    }
    inline bool as_bool() const {
        auto& s = as_string();
        if (s == "true") return true;
        if (s == "false") return false;
        throw std::runtime_error(std::string("option with string value '")
            + s
            + "' can not be converted to an boolean value!");
    }
    template<class T>
    inline T as() const {
        return lexical_cast<T>(as_string());
    }
};

}
#endif
