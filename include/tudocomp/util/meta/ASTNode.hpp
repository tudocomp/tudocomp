#pragma once

#include <glog/logging.h>

#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace tdc {
namespace meta {
namespace ast {

/// \brief Represents a node in an abstract syntax tree (AST).
class Node {
private:
    std::string m_name;
    std::string m_value; // only valid if leaf

    std::vector<Node> m_keyless_children;
    std::map<std::string, Node> m_children;

    inline size_t num_children() const {
        return (m_keyless_children.size() + m_children.size());
    }

public:
    inline Node(const std::string& name,
                const std::string& value = "") : m_name(name),
                                                 m_value(value) {
    }

    inline Node(const Node& other)
        : m_name(other.m_name),
          m_value(other.m_value),
          m_keyless_children(other.m_keyless_children),
          m_children(other.m_children) {
    }

    inline Node(Node&& other)
        : m_name(std::move(other.m_name)),
          m_value(std::move(other.m_value)),
          m_keyless_children(std::move(other.m_keyless_children)),
          m_children(std::move(other.m_children)) {
    }

    inline bool is_leaf() const {
        return num_children() == 0;
    }

    inline const std::string& name() const { return m_name; }
    inline const std::string& value() const {
        CHECK(is_leaf()) << "only leaves have a value";
        return m_name;
    }

    inline void add_child(Node&& node) {
        m_keyless_children.emplace_back(std::move(node));
    }

    inline void add_child(const std::string& key, Node&& node) {
        m_children.emplace(key, std::move(node));
    }

    inline void value(std::string&& v) {
        m_value = v;
    }

    inline std::string str() const {
        std::stringstream ss;
        ss << m_name;

        if(is_leaf()) {
            if(!m_value.empty()) {
                ss << "'" << m_value << "'";
            }
        } else {
            ss << "(";

            size_t i = 0;

            for(auto& child : m_keyless_children) {
                ss << child.str();
                if(++i < num_children()) ss << ", ";
            }

            for(auto& e : m_children) {
                ss << e.first << "=" << e.second.str();
                if(++i < num_children()) ss << ", ";
            }

            ss << ")";
        }

        return ss.str();
    }
};

}}} //ns
