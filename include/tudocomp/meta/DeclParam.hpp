#pragma once

#include <glog/logging.h>

#include <string>
#include <utility>

namespace tdc {
namespace meta {
namespace decl {

/// \brief Represents an algorithm parameter.
class Param {
private:
    std::string m_name;
    bool m_primitive;
    bool m_list;
    std::string m_type; // only valid if non-primitive

public:
    /// \brief Main constructor.
    inline Param(
        const std::string& name,
        bool primitive = true,
        bool list = false,
        const std::string& type = "") : m_name(name),
                                        m_primitive(false),
                                        m_list(list) {

        CHECK(primitive || !type.empty()) <<
            "non-primitive parameters must have a type";

        CHECK(!primitive || type.empty()) <<
            "primitive parameters must not have a type";

        m_type = primitive ? "$" : type;
    }

    /// \brief Copy constructor.
    inline Param(const Param& other) : m_name(other.m_name),
                                       m_primitive(other.m_primitive),
                                       m_list(other.m_list),
                                       m_type(other.m_type) {
    }

    /// \brief Move constructor.
    inline Param(Param&& other) : m_name(std::move(other.m_name)),
                                  m_primitive(std::move(other.m_primitive)),
                                  m_list(std::move(other.m_list)),
                                  m_type(std::move(other.m_type)) {
    }

    inline const std::string& name() const { return m_name; }
    inline bool is_primitive() const { return m_primitive; }
    inline bool is_list() const { return m_list; }
    inline const std::string& type() const { return m_type; }

    inline const std::string str() const {
        return name() + " : " + (is_list() ? "[" + type() + "]" : type());
    }
};

}}} //ns
