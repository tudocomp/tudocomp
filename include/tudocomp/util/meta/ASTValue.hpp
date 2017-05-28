#pragma once

#include <string>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Abstract base for AST values.
class Value {
public:
    virtual std::string str() const = 0;
};

}}} //ns
