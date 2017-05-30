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
    std::vector<std::shared_ptr<Node>> m_values;

public:
    inline List() {
    }

    inline void add_value(const std::shared_ptr<Node> value) {
        m_values.emplace_back(value);
    }

    inline std::vector<const Node*> items() const {
        std::vector<const Node*> items;
        for(auto& v : m_values) items.emplace_back(v.get());
        return items;
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
        return "list";
    }
};

}}} //ns
