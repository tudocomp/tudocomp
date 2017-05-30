#pragma once

#include <map>
#include <memory>
#include <vector>
#include <sstream>

namespace tdc {

/// \brief Contains a basic JSON builder implementation.
namespace json {

/// \cond INTERNAL
inline void level_indent(std::ostream& s, unsigned int level) {
    while(level--) s << "    ";
}
/// \endcond

/// \brief Represents a single value that can be represented as a JSON string.
class Value {
public:
    /// \brief Writes this value's JSON string representation into the given
    ///        output stream.
    ///
    /// \param s the output stream to write to
    /// \param level the indentation level
    virtual void str(std::ostream& s, unsigned int level = 0) const = 0;

    /// \brief Returns this value's JSON string representation.
    ///
    /// \return this value's JSON string representation
    inline std::string str() const {
        std::ostringstream oss;
        str(oss);
        return oss.str();
    }
};

/// \brief Template for containers of values that are supported by the
///        \c std::ostream left shift operator for string conversion.
///
/// \tparam T the contained value type
template<typename T>
class TValue : public Value {
private:
    T m_value;

public:
    /// \brief Constructs a value container.
    inline TValue(const T& value) : m_value(value) {};

    /// \brief Yields the JSON string representation of the contained value.
    ///
    /// \return the string representation of the contained value
    virtual inline void str(
        std::ostream& s, unsigned int = 0) const override {

        s << m_value;
    }
};

/// \cond INTERNAL
const char        quote_char = '\"';
const std::string quote_escape = "\\\"";

template<>
inline void TValue<char>::str(std::ostream& s, unsigned int) const {
    s << quote_char;

    if(m_value == quote_char) {
        s << quote_escape;
    } else {
        s << m_value;
    }

    s << quote_char;
}

template<>
inline TValue<std::string>::TValue(const std::string& value) {
    m_value = std::string(value);

    // escape quote character
    size_t pos = 0;
    for(size_t x = m_value.find(quote_char);
        x != std::string::npos;
        x = m_value.find(quote_char, pos)) {

        m_value.replace(x, 1, quote_escape);
        pos = x+2;
    }
}

template<>
inline void TValue<std::string>::str(std::ostream& s, unsigned int) const {
    s << quote_char << m_value << quote_char;
}
/// \endcond

class Object;

/// \brief Represents an array of values.
class Array : public Value {
private:
    std::vector<std::shared_ptr<Value>> m_values;

public:
    /// \brief Adds a value to the array.
    ///
    /// \tparam T the value type
    /// \param value the value to add
    template<typename T>
    inline void add(const T& value) {
        m_values.push_back(std::make_shared<TValue<T>>(value));
    }

    /// \brief Adds a C string value to the array.
    ///
    /// \param value the C string value to add
    inline void add(const char* value) {
        add(std::string(value));
    }

    /// \brief Adds an array object to the array.
    ///
    /// \param value value the array object to add
    inline void add(const Array& value) {
        m_values.push_back(std::make_shared<Array>(value));
    }

    /// \brief Adds an object to the array.
    ///
    /// \param obj the object to add
    void add(const Object& obj); //defined below

    /// \brief Writes the array's JSON string representation into the given
    ///        stream.
    ///
    /// \param s the output stream to write to
    /// \param level the indentation level
    virtual inline void str(std::ostream& s, unsigned int level = 0) const override {
        s << '[';
        auto end = m_values.end();
        for(auto it = m_values.begin(); it != end; ++it) {
            (*it)->str(s, level);
            if(std::next(it) != end) s << ',';
        }
        s << ']';
    }
};

/// \brief Represents a JSON object that behaves like a dictionary.
class Object : public Value {
private:
    std::map<std::string, std::shared_ptr<Value>> m_fields;

public:
    /// \brief Sets a member value of the object.
    ///
    /// \tparam the value type
    /// \param key the member name
    /// \param value the value
    template<typename T>
    inline void set(const std::string& key, const T& value) {
        m_fields[key] = std::make_shared<TValue<T>>(value);
    }

    /// \brief Sets a member value of the object.
    ///
    /// \param key the member name
    /// \param value the C string value
    inline void set(const std::string& key, const char* value) {
        set(key, std::string(value));
    }

    /// \brief Sets a member value of the object.
    ///
    /// \param key the member name
    /// \param value the object
    inline void set(const std::string& key, const Object& value) {
        m_fields[key] = std::make_shared<Object>(value);
    }

    /// \brief Sets a member value of the object.
    ///
    /// \param key the member name
    /// \param value the array
    inline void set(const std::string& key, const Array& value) {
        m_fields[key] = std::make_shared<Array>(value);
    }

    /// \brief Writes the object's JSON string representation into the given
    ///        stream.
    ///
    /// \param s the output stream to write to
    /// \param level the indentation level
    virtual inline void str(std::ostream& s, unsigned int level = 0) const override {
        s << '{' << std::endl;

        auto end = m_fields.end();
        for(auto it = m_fields.begin(); it != end; ++it) {
            level_indent(s, level + 1);

            s << '\"' << it->first << "\": ";
            it->second->str(s, level + 1);

            if(std::next(it) != end) s << ',';
            s << std::endl;
        }

        level_indent(s, level);
        s << '}';
    }

    /// \brief Returns the object's JSON string representation.
    ///
    /// \return the object's JSON string representation
    inline std::string str() const {
        return Value::str();
    }
};

/// \cond INTERNAL
inline void Array::add(const Object& value) {
    m_values.push_back(std::make_shared<Object>(value));
}
/// \endcond

}} //ns
