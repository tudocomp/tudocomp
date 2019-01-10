#pragma once

#include <tudocomp/meta/ast/Node.hpp>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Error type for type conversion related errors.
class TypeMismatchError : public std::runtime_error {
public:
    inline TypeMismatchError(
        const std::string& msg,
        const std::string& expected,
        const std::string& got)
        : std::runtime_error(msg + " (expected "
        + expected + ", got " + got + ")") {
    }
};

inline std::string safe_get_debug_type(NodePtr<> node) {
    return node ? node->debug_type() : "none";
}

template<typename node_t>
inline NodePtr<node_t> convert(
    NodePtr<> node,
    const std::string& err = "failed to convert") {

    auto converted = node_cast<node_t>(node);
    if(!converted) {
        throw TypeMismatchError(
            err,
            node_t::ast_debug_type(),
            safe_get_debug_type(node));
    } else {
        return converted;
    }
}

}}} //ns
