#pragma once

#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#include <glog/logging.h>

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

// Forward declaration to be used by the parser
/// \cond INTERNAL
namespace pattern {
    class Algorithm;

    inline std::ostream& operator<<(std::ostream&, const Algorithm&);
}
/// \endcond

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
    ds::InputRestrictionsAndFlags m_ds_flags;
public:
    inline ~AlgorithmValue();
    inline AlgorithmValue(const AlgorithmValue& other);
    inline AlgorithmValue(AlgorithmValue&& other);
    inline AlgorithmValue(std::string&& name,
                          ArgumentMap&& arguments,
                          std::unique_ptr<pattern::Algorithm>&& static_selection,
                          ds::InputRestrictionsAndFlags flags);

    inline const std::string& name() const;
    inline const ArgumentMap& arguments() const;
    inline ds::InputRestrictionsAndFlags textds_flags() const;
    inline const pattern::Algorithm& static_selection() const;
    inline AlgorithmValue& operator=(AlgorithmValue&& other);
};

inline std::ostream& operator<<(std::ostream& os, const AlgorithmValue& av);

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
    inline double as_floating() const;
    inline bool as_bool() const;
    template<class T>
    inline T as() const;
    inline OptionValue& operator=(OptionValue&& other);
};

inline std::ostream& operator<<(std::ostream& os, const OptionValue& ov);

inline AlgorithmValue::~AlgorithmValue() {}

inline AlgorithmValue::AlgorithmValue(const AlgorithmValue& other):
    m_name(other.m_name),
    m_arguments(other.m_arguments),
    m_ds_flags(other.m_ds_flags)
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
    m_ds_flags(other.m_ds_flags) {}

inline AlgorithmValue::AlgorithmValue(std::string&& name,
                        ArgumentMap&& arguments,
                        std::unique_ptr<pattern::Algorithm>&& static_selection,
                        ds::InputRestrictionsAndFlags flags):
    m_name(std::move(name)),
    m_arguments(std::move(arguments)),
    m_static_selection(std::move(static_selection)),
    m_ds_flags(flags) {}

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
    this->m_ds_flags = std::move(other.m_ds_flags);
    return *this;
}
inline ds::InputRestrictionsAndFlags AlgorithmValue::textds_flags() const {
    return m_ds_flags;
}

inline OptionValue::~OptionValue() {}

inline OptionValue::OptionValue(): OptionValue("") {}
inline OptionValue::OptionValue(std::string&& value):
    m_is_value(true),
    m_value_or_algorithm(AlgorithmValue(std::move(value), {}, std::make_unique<pattern::Algorithm>(), ds::InputRestrictionsAndFlags())) {}
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
inline double OptionValue::as_floating() const {
    return lexical_cast<double>(as_string());
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

inline std::ostream& operator<<(std::ostream& os, const AlgorithmValue& av) {
    os << av.name() << " {";

    for (auto& e : av.arguments()) {
        os << e.first << " : " << e.second << ", ";
    }

    return os << "; ds_flags: " << av.textds_flags()
        << ", static_selection: " << av.static_selection()
        << "}";
}

inline std::ostream& operator<<(std::ostream& os, const OptionValue& ov) {
    if (!ov.is_algorithm()) {
        return os << "string: " << ov.as_string();
    } else {
        return os << ov.as_algorithm();
    }
}

}

