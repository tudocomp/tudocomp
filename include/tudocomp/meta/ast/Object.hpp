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
    NodePtr<> m_value;

public:
    /// \brief Main constructor.
    /// \param name the parameter name
    /// \param value the assigned node
    inline Param(const std::string& name,
                 const NodePtr<> value)
                 : m_name(name), m_value(value) {
    }

    /// \brief Constructs an unnamed parameter.
    /// \param value the assigned node
    inline Param(const NodePtr<> value) : Param("", value) {
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
    inline NodePtr<> value() const {
        return m_value;
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
    std::vector<Param> m_params;

public:
    static inline std::string ast_debug_type() {
        return "object";
    }

    /// \brief Main constructor.
    /// \param name the object's name
    inline Object(const std::string& name) : m_name(name) {
    }

    /// \brief Copy constructor.
    /// \param other the object to copy
    inline Object(const Object& other)
        : m_name(other.m_name),
          m_params(other.m_params) {
    }

    /// \brief Move constructor.
    /// \param other the object to move
    inline Object(Object&& other)
        : m_name(std::move(other.m_name)),
          m_params(std::move(other.m_params)) {
    }

    /// \brief Adds a parameter to the object.
    /// \param param the parameter to add.
    inline void add_param(Param&& param) {
        m_params.emplace_back(std::move(param));
    }

    /// \brief Gets the object's name.
    /// \return the object's name
    inline const std::string& name() const { return m_name; }

    /// \brief Returns a read-only vector of the object's parameters.
    /// \return a read-only vector of the object's parameters
    inline const std::vector<Param>& params() const {
        return m_params;
    }

    /// \brief Construct an object that inherits from a base object.
    ///
    /// The returned object starts as a copy of the current object.
    /// Subsequently, all parameters from the base object are added, unless
    /// they are already present.
    ///
    /// \param base the base object
    /// \return the enhanced object
    inline NodePtr<Object> inherit(NodePtr<Object> base) const {
        auto obj = std::make_shared<Object>(*this);
        for(auto& p : base->params()) {
            if(!obj->has_param(p.name())) {
                obj->add_param(ast::Param(p));
            }
        }
        return obj;
    }

    /// \brief Tests if the object has a parameter with the given name
    /// \param name the parameter in question
    /// \return \c true iff the object has a parameter with the given name
    inline bool has_param(const std::string& name) const {
        for(auto& p : m_params) {
            if(p.name() == name) return true;
        }
        return false;
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

    virtual std::string debug_type() const override {
        return ast_debug_type();
    }
};

}}} //ns
