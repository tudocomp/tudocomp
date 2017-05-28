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

    virtual std::string str() const override {
        return '\'' + m_value + '\'';
    }
};

}}} //ns
