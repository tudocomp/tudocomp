#pragma once

#include <tudocomp/meta/ast/Node.hpp>

#include <memory>
#include <sstream>
#include <vector>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Represents a named value or a value assignment.
class Param {
private:
    std::string m_name;
    std::shared_ptr<Node> m_value;

public:
    inline Param(const std::string& name,
                 const std::shared_ptr<Node> value)
                 : m_name(name), m_value(value) {
    }

    inline Param(const std::shared_ptr<Node> value) : Param("", value) {
    }

    inline Param(const Param& other)
        : m_name(other.m_name), m_value(other.m_value) {
    }

    inline bool has_name() const { return !m_name.empty(); }

    inline const std::string& name() const { return m_name; }
    inline std::shared_ptr<const Node> value() const { return m_value; }

    inline std::string str() const {
        std::stringstream ss;
        if(!m_name.empty()) ss << m_name << '=';
        ss << m_value->str();
        return ss.str();
    }
};

/// \brief Represents an object, holding a list of parameters.
class Object : public Node {
private:
    std::string m_name;
    std::vector<Param> m_params;

public:
    inline Object(const std::string& name) : m_name(name) {
    }

    inline void add_param(const Param& param) {
        m_params.emplace_back(param);
    }

    inline const std::string& name() const { return m_name; }
    inline std::vector<const Param*> params() const {
        std::vector<const Param*> params;
        for(auto& p : m_params) params.emplace_back(&p);
        return params;
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
