#pragma once

#include <glog/logging.h>

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Abstract base for AST values.
class Value {
public:
    virtual std::string str() const = 0;
};

/// \brief Represents a single primitive value in an AST.
class Primitive : public Value {
private:
    std::string m_value;

public:
    inline Primitive(const std::string& value) : m_value(value) {
    }

    virtual std::string str() const override {
        return '\'' + m_value + '\'';
    }
};

/// \brief Represents a list of values.
class List : public Value {
private:
    std::vector<std::shared_ptr<Value>> m_values;

public:
    inline List() {
    }

    inline void add_value(const std::shared_ptr<Value> value) {
        m_values.emplace_back(value);
    }

    virtual std::string str() const override {
        std::stringstream ss;
        ss << '[';
        size_t i = 0;
        for(auto& v : m_values) {
            ss << v->str();
            if(++i < m_values.size()) ss << ", ";
        }
        ss << ']';
        return ss.str();
    }
};

/// \brief Represents a named value or a value assignment.
class Param {
private:
    std::string m_name;
    std::shared_ptr<Value> m_value;

public:
    inline Param(const std::string& name,
                 const std::shared_ptr<Value> value)
                 : m_name(name), m_value(value) {
    }

    inline Param(const std::shared_ptr<Value> value) : Param("", value) {
    }

    inline Param(const Param& other)
        : m_name(other.m_name), m_value(other.m_value) {
    }

    inline std::string str() const {
        std::stringstream ss;
        if(!m_name.empty()) ss << m_name << '=';
        ss << m_value->str();
        return ss.str();
    }
};

/// \brief Represents a named list of parameters.
class Node : public Value {
private:
    std::string m_name;
    std::vector<Param> m_params;

public:
    inline Node(const std::string& name) : m_name(name) {
    }

    inline void add_param(const Param& param) {
        m_params.emplace_back(param);
    }

    virtual std::string str() const override {
        std::stringstream ss;
        ss << m_name << '(';
        size_t i = 0;
        for(auto& p : m_params) {
            ss << p.str();
            if(++i < m_params.size()) ss << ", ";
        }
        ss << ')';
        return ss.str();
    }
};

}}} //ns
