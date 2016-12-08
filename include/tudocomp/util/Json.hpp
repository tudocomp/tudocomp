#ifndef _INCLUDED_JSON_HPP_
#define _INCLUDED_JSON_HPP_

#include <map>
#include <memory>
#include <sstream>

namespace tdc {
namespace json {

inline void level_indent(std::ostream& s, unsigned int level) {
    while(level--) s << "    ";
}

class Value {
public:
    virtual void str(std::ostream& s, unsigned int level = 0) const = 0;

    inline std::string str() const {
        std::ostringstream oss;
        str(oss);
        return oss.str();
    }
};

template<typename T>
class TValue : public Value {
private:
    T m_value;

public:
    TValue(const T& value) : m_value(value) {};

    virtual inline void str(
        std::ostream& s, unsigned int level = 0) const override {

        s << m_value;
    }
};

template<>
inline void TValue<char>::str(std::ostream& s, unsigned int level) const {
    s << '\"' << m_value << '\"';
}

template<>
inline void TValue<std::string>::str(std::ostream& s, unsigned int level) const {
    s << '\"' << m_value << '\"';
}

class Object;
class Array : public Value {
private:
    std::vector<std::shared_ptr<Value>> m_values;

public:
    template<typename T>
    inline void add(const T& value) {
        m_values.push_back(std::make_shared<TValue<T>>(value));
    }

    inline void add(const char* value) {
        add(std::string(value));
    }

    inline void add(const Array& value) {
        m_values.push_back(std::make_shared<Array>(value));
    }

    void add(const Object&); //defined below

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

class Object : public Value {
private:
    std::map<std::string, std::shared_ptr<Value>> m_fields;

public:
    template<typename T>
    inline void set(const std::string& key, const T& value) {
        m_fields[key] = std::make_shared<TValue<T>>(value);
    }

    inline void set(const std::string& key, const char* value) {
        set(key, std::string(value));
    }

    inline void set(const std::string& key, const Object& value) {
        m_fields[key] = std::make_shared<Object>(value);
    }

    inline void set(const std::string& key, const Array& value) {
        m_fields[key] = std::make_shared<Array>(value);
    }

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
};

inline void Array::add(const Object& value) {
    m_values.push_back(std::make_shared<Object>(value));
}

}} //ns

#endif
