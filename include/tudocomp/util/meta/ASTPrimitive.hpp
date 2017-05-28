#pragma once

#include <tudocomp/util/meta/ASTValue.hpp>

namespace tdc {
namespace meta {
namespace ast {

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

}}} //ns
