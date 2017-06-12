#pragma once

#include <tudocomp/meta/ast/Node.hpp>

#include <memory>
#include <sstream>
#include <vector>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Represents a named node, a.k.a. a parameter.
class Param {
private:
    std::string m_name;
    std::shared_ptr<Node> m_value;

public:
    /// \brief Main constructor.
    /// \param name the parameter name
    /// \param value the assigned node
    inline Param(const std::string& name,
                 const std::shared_ptr<Node> value)
                 : m_name(name), m_value(value) {
    }

    /// \brief Constructs an unnamed parameter.
    /// \param value the assigned node
    inline Param(const std::shared_ptr<Node> value) : Param("", value) {
    }

    /// \brief Copy constructor.
    /// \param other the object to copy from
    inline Param(const Param& other)
        : m_name(other.m_name), m_value(other.m_value) {
    }

    /// \brief Move constructor.
    /// \param other the object to move
    inline Param(Param&& other)
        : m_name(std::move(other.m_name)), m_value(std::move(other.m_value)) {
    }

    /// \brief Tests if the parameter has a name.
    /// \return \c true if the name is non-empty, \c false otherwise.
    inline bool has_name() const { return !m_name.empty(); }

    /// \brief Gets the parameter's name.
    ///
    /// If the parameter is unnamed, the returned name will be an empty string.
    ///
    /// \return the parameter's name
    inline const std::string& name() const { return m_name; }

    /// \brief Gets the parameter's assigned node.
    /// \return the parameter's assigned node
    inline std::shared_ptr<const Node> value() const {
        return std::const_pointer_cast<const Node>(m_value);
    }

    /// \brief Returns a human-readable string representation of the parameter.
    /// \return a human-readable string representation of the parameter
    inline std::string str() const {
        std::stringstream ss;
        if(!m_name.empty()) ss << m_name << '=';
        ss << m_value->str();
        return ss.str();
    }
};

/// \brief Represents an object, a named node holding a list of parameters.
class Object : public Node {
private:
    std::string m_name;
    std::vector<std::shared_ptr<const Param>> m_params;

public:
    /// \brief Main constructor.
    /// \param name the object's name
    inline Object(const std::string& name) : m_name(name) {
    }

    /// \brief Adds a parameter to the object.
    /// \param param the parameter to add.
    inline void add_param(std::shared_ptr<const Param> param) {
        m_params.emplace_back(param);
    }

    /// \brief Gets the object's name.
    /// \return the object's name
    inline const std::string& name() const { return m_name; }

    /// \brief Returns a read-only vector of the object's parameters.
    /// \return a read-only vector of the object's parameters
    inline const std::vector<std::shared_ptr<const Param>>& params() const {
        return m_params;
    }

    virtual std::string str() const override {
        std::stringstream ss;
        ss << m_name << '(';
        size_t i = 0;
        for(auto& p : m_params) {
            ss << p->str();
            if(++i < m_params.size()) ss << ", ";
        }
        ss << ')';
        return ss.str();
    }

    virtual std::string debug_type() const override {
        return "object";
    }
};

}}} //ns
