#pragma once

#include <string>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Abstract base for AST nodes.
class Node {
public:
    /// \brief Returns a human-readable string representation of the node.
    /// \return a human-readable string representation of the node
    virtual std::string str() const = 0;

    /// \brief Returns a human-readable type identifier for this node.
    /// \return a human-readable type identifier for this node.
    virtual std::string debug_type() const = 0;
};

}}} //ns
