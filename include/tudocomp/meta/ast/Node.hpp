#pragma once

#include <string>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Abstract base for AST nodes.
class Node {
public:
    virtual std::string str() const = 0;
    virtual std::string debug_type() const = 0;
};

}}} //ns
