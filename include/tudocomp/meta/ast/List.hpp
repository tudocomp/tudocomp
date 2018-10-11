#pragma once

#include <tudocomp/meta/ast/Node.hpp>

#include <memory>
#include <sstream>
#include <vector>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Represents a list of nodes.
class List : public Node {
private:
    std::vector<std::shared_ptr<const Node>> m_values;

public:
    static inline std::string ast_debug_type() {
        return "list";
    }

    /// \brief Main constructor.
    inline List() {
    }

    /// \brief Adds a node to the list.
    /// \param value the node to add to the list
    inline void add_value(std::shared_ptr<const Node> value) {
        m_values.emplace_back(value);
    }

    /// \brief Yields a read-only vector of the list's items.
    /// \return a read-only vector of the list's items
    inline const std::vector<std::shared_ptr<const Node>>& items() const {
        return m_values;
    }

    virtual std::string str() const override {
        std::stringstream ss;
        ss << '[';
        size_t i = 0;
        for(auto& v : m_values) {
            ss << v->str();
            if(++i < m_values.size()) ss << ", ";
        }
        ss << ']';
        return ss.str();
    }

    virtual std::string debug_type() const override {
        return ast_debug_type();
    }
};

}}} //ns
