#pragma once

#include <tudocomp/meta/ast/Node.hpp>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Represents a single value in an AST.
class Value : public Node {
private:
    std::string m_value;

public:
    static inline std::string ast_debug_type() {
        return "value";
    }

    /// \brief Main constructor.
    /// \param value the contained string value
    inline Value(const std::string& value) : m_value(value) {
    }

    /// \brief Gets the value contained in this value node.
    /// \return the contained string value
    const std::string& value() const { return m_value; }

    virtual std::string str() const override {
        return '\'' + m_value + '\'';
    }

    virtual std::string debug_type() const override {
        return ast_debug_type();
    }
};

}}} //ns
