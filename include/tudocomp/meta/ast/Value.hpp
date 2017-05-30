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
    inline Value(const std::string& value) : m_value(value) {
    }

    const std::string& value() const { return m_value; }

    virtual std::string str() const override {
        return '\'' + m_value + '\'';
    }

    virtual std::string debug_type() const override {
        return "value";
    }
};

}}} //ns
