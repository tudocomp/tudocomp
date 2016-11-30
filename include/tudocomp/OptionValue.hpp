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

#include <tudocomp/util.hpp>

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
    bool m_needs_sentinel;
public:
    inline ~AlgorithmValue();
    inline AlgorithmValue(const AlgorithmValue& other);
    inline AlgorithmValue(AlgorithmValue&& other);
    inline AlgorithmValue(std::string&& name,
                          ArgumentMap&& arguments,
                          std::unique_ptr<pattern::Algorithm>&& static_selection,
                          bool needs_sentinel);

    inline const std::string& name() const;
    inline const ArgumentMap& arguments() const;
    inline bool needs_sentinel_terminator() const;
    inline const pattern::Algorithm& static_selection() const;
    inline AlgorithmValue& operator=(AlgorithmValue&& other);
};

class OptionValue {
    bool m_is_value;
    AlgorithmValue m_value_or_algorithm;
    friend class AlgorithmValue;
public:
    inline ~OptionValue();
    inline OptionValue();
    inline OptionValue(std::string&& value);
    inline OptionValue(AlgorithmValue&& algorithm);
    inline OptionValue(const OptionValue& other);
    inline OptionValue(OptionValue&& other);

    inline bool is_algorithm() const;
    inline const AlgorithmValue& as_algorithm() const;
    inline AlgorithmValue to_algorithm() &&;
    inline const std::string& as_string() const;
    inline uint64_t as_integer() const;
    inline bool as_bool() const;
    template<class T>
    inline T as() const;
    inline OptionValue& operator=(OptionValue&& other);
};

inline AlgorithmValue::~AlgorithmValue() {}

inline AlgorithmValue::AlgorithmValue(const AlgorithmValue& other):
    m_name(other.m_name),
    m_arguments(other.m_arguments),
    m_needs_sentinel(other.m_needs_sentinel)
{
    if (other.m_static_selection != nullptr) {
        m_static_selection = std::make_unique<pattern::Algorithm>(
            *other.m_static_selection);
    }
}

inline AlgorithmValue::AlgorithmValue(AlgorithmValue&& other):
    m_name(std::move(other.m_name)),
    m_arguments(std::move(other.m_arguments)),
    m_static_selection(std::move(other.m_static_selection)),
    m_needs_sentinel(other.m_needs_sentinel) {}

inline AlgorithmValue::AlgorithmValue(std::string&& name,
                        ArgumentMap&& arguments,
                        std::unique_ptr<pattern::Algorithm>&& static_selection,
                        bool needs_sentinel
                                     ):
    m_name(std::move(name)),
    m_arguments(std::move(arguments)),
    m_static_selection(std::move(static_selection)),
    m_needs_sentinel(needs_sentinel) {}

inline const std::string& AlgorithmValue::name() const {
    return m_name;
}
inline const AlgorithmValue::ArgumentMap& AlgorithmValue::arguments() const {
    return m_arguments;
}
inline const pattern::Algorithm& AlgorithmValue::static_selection() const {
    CHECK(m_static_selection != nullptr);
    return *m_static_selection;
}
inline AlgorithmValue& AlgorithmValue::operator=(AlgorithmValue&& other) {
    this->m_name = std::move(other.m_name);
    this->m_arguments = std::move(other.m_arguments);
    this->m_static_selection = std::move(other.m_static_selection);
    this->m_needs_sentinel = std::move(other.m_needs_sentinel);
    return *this;
}
inline bool AlgorithmValue::needs_sentinel_terminator() const {
    return m_needs_sentinel;
}

inline OptionValue::~OptionValue() {}

inline OptionValue::OptionValue(): OptionValue("") {}
inline OptionValue::OptionValue(std::string&& value):
    m_is_value(true),
    m_value_or_algorithm(AlgorithmValue(std::move(value), {}, std::make_unique<pattern::Algorithm>(), false)) {}
inline OptionValue::OptionValue(AlgorithmValue&& algorithm):
    m_is_value(false),
    m_value_or_algorithm(std::move(algorithm)) {}

inline OptionValue::OptionValue(const OptionValue& other):
    m_is_value(other.m_is_value),
    m_value_or_algorithm(other.m_value_or_algorithm) {}

inline OptionValue::OptionValue(OptionValue&& other):
    m_is_value(other.m_is_value),
    m_value_or_algorithm(std::move(other.m_value_or_algorithm)) {}

inline bool OptionValue::is_algorithm() const {
    return !m_is_value;
}
inline const AlgorithmValue& OptionValue::as_algorithm() const {
    CHECK(is_algorithm());
    return m_value_or_algorithm;
}

inline AlgorithmValue OptionValue::to_algorithm() && {
    CHECK(is_algorithm());
    return std::move(m_value_or_algorithm);
}

inline const std::string& OptionValue::as_string() const {
    CHECK(m_is_value);
    return m_value_or_algorithm.m_name;
}
inline uint64_t OptionValue::as_integer() const {
    return lexical_cast<uint64_t>(as_string());
}
inline bool OptionValue::as_bool() const {
    auto& s = as_string();
    if (s == "true") return true;
    if (s == "false") return false;
    throw std::runtime_error(std::string("option with string value '")
        + s
        + "' can not be converted to an boolean value!");
}
template<class T>
inline T OptionValue::as() const {
    return lexical_cast<T>(as_string());
}
inline OptionValue& OptionValue::operator=(OptionValue&& other) {
    this->m_is_value = other.m_is_value;
    this->m_value_or_algorithm = std::move(other.m_value_or_algorithm);
    return *this;
}


}
#endif
