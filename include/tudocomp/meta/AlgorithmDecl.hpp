#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tdc {
namespace meta {

/// \brief Represents an algorithm declaration.
class AlgorithmDecl {
public:
    /// \brief Represents a declared parameter for an algorithm.
    class Param {
    private:
        std::string m_name;
        bool m_primitive; // if false, type needs to be set
        bool m_list; // if true, value/type flags account for list items
        std::string m_type; // only valid if non-primitive

    public:
        /// \brief Main constructor.
        inline Param(
            const std::string& name,
            bool primitive = true,
            bool list = false,
            const std::string& type = "")
            : m_name(name), m_primitive(primitive), m_list(list) {

            CHECK(primitive || !type.empty()) <<
                "non-primitive parameters must have a type";

            CHECK(!primitive || type.empty()) <<
                "primitive parameters must not have a type";

            m_type = primitive ? "$" : type;
        }

        /// \brief Copy constructor.
        inline Param(const Param& other)
            : m_name(other.m_name),
              m_primitive(other.m_primitive),
              m_list(other.m_list),
              m_type(other.m_type) {
        }

        /// \brief Move constructor.
        inline Param(Param&& other)
            : m_name(std::move(other.m_name)),
              m_primitive(other.m_primitive),
              m_list(other.m_list),
              m_type(std::move(other.m_type)) {
        }

        inline const std::string& name() const { return m_name; }
        inline bool is_primitive() const { return m_primitive; }
        inline bool is_list() const { return m_list; }
        inline const std::string& type() const { return m_type; }

        inline const std::string str() const {
            return m_name + " : " + (m_list ? "[" + m_type + "]" : m_type);
        }
    };

private:
    std::string m_name;
    std::string m_type;
    std::string m_desc;
    std::vector<Param> m_params;

public:
    /// \brief Main constructor.
    inline AlgorithmDecl(
        const std::string& name,
        const std::string& type,
        const std::string& desc = "")
        : m_name(name), m_type(type), m_desc(desc) {
    }

    /// \brief Copy constructor.
    inline AlgorithmDecl(const AlgorithmDecl& other)
        : m_name(other.m_name),
          m_type(other.m_type),
          m_desc(other.m_desc),
          m_params(other.m_params) {
    }

    /// \brief Move constructor.
    inline AlgorithmDecl(AlgorithmDecl&& other)
        : m_name(std::move(other.m_name)),
          m_type(std::move(other.m_type)),
          m_desc(std::move(other.m_desc)),
          m_params(std::move(other.m_params)) {
    }

    inline void add_param(Param&& p) {
        m_params.emplace_back(std::move(p));
    }

    inline const std::string& name() const { return m_name; }
    inline const std::string& type() const { return m_type; }
    inline const std::string& desc() const { return m_desc; }
    inline const std::vector<Param>& params() const {
        return m_params;
    }

    inline std::string str() const {
        std::stringstream ss;
        ss << m_name << "(";

        size_t i = 0;
        for(auto& param : m_params) {
            ss << param.str();
            if(++i < m_params.size()) ss << ", ";
        }

        ss << ") : " << m_type;
        return ss.str();
    }
};

using AlgorithmDict = std::unordered_map<std::string, AlgorithmDecl>;

}} //ns
