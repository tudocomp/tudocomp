#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tudocomp/meta/ast/Node.hpp>
#include <tudocomp/meta/ast/List.hpp>
#include <tudocomp/meta/ast/Value.hpp>
#include <tudocomp/meta/ast/Object.hpp>
#include <tudocomp/meta/ast/TypeConversion.hpp>

namespace tdc {
namespace meta {

/// \brief Error type for declaration related errors.
class DeclError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// \brief Represents an algorithm declaration.
class AlgorithmDecl {
public:
    /// \brief Represents a declared parameter for an algorithm.
    class Param {
    private:
        std::string m_name;
        bool m_primitive;   // if false, type needs to be set
        bool m_list;        // if true, value/type flags account for list items
        std::string m_type; // only valid if non-primitive

        // default value - only used if primitive
        ast::NodePtr<> m_default;

    public:
        /// \brief Main constructor.
        /// \param name the parameter name
        /// \param primitive if \c true, the parameter accepts string values,
        ///                  otherwise it accepts a sub algorithm
        /// \param list if \c true, the parameter accepts a list of values
        /// \param type if the parameter is non-primitive, sub algorithms must
        ///             match this type
        inline Param(
            const std::string& name,
            bool primitive = true,
            bool list = false,
            const std::string& type = "",
            ast::NodePtr<> defv = ast::NodePtr<>())
            : m_name(name),
              m_primitive(primitive),
              m_list(list),
              m_default(defv) {

            if(!primitive && type.empty()) {
                throw DeclError("non-primitive parameters must have a type");
            }

            if(primitive && !type.empty()) {
                throw DeclError("primitive parameters must not have a type");
            }

            m_type = primitive ? "$" : type;

            // sanity checks on default value
            if(defv) {
                if(list) {
                    auto list_value = ast::convert<ast::List>(defv,
                        "default value type mismatch");

                    // check each list item
                    if(primitive) {
                        for(auto& item : list_value->items()) {
                            ast::convert<ast::Value>(item,
                                "default list item type mismatch");
                        }
                    } else {
                        for(auto& item : list_value->items()) {
                            ast::convert<ast::Object>(item,
                                "default list item type mismatch");
                        }
                    }
                } else if(primitive) {
                    ast::convert<ast::Value>(defv,
                        "default value type mismatch");
                } else {
                    ast::convert<ast::Object>(defv,
                        "default value type mismatch");
                }
            }
        }

        /// \brief Copy constructor.
        /// \param other the object to copy
        inline Param(const Param& other)
            : m_name(other.m_name),
              m_primitive(other.m_primitive),
              m_list(other.m_list),
              m_type(other.m_type),
              m_default(other.m_default) {
        }

        /// \brief Move constructor.
        /// \param other the object to move
        inline Param(Param&& other)
            : m_name(std::move(other.m_name)),
              m_primitive(other.m_primitive),
              m_list(other.m_list),
              m_type(std::move(other.m_type)),
              m_default(std::move(other.m_default)) {
        }

        inline const std::string& name() const { return m_name; }
        inline bool is_primitive() const { return m_primitive; }
        inline bool is_list() const { return m_list; }
        inline const std::string& type() const { return m_type; }

        inline void check_type(ast::NodePtr<> node) {
            // TODO:

            // If primitive, make sure the node is an ast::Value.

            // Otherwise, make sure the node is an ast::Object AND
            // also check that the algorithm exists and its type matches.

            // In case of a list, perform this check for each item.
        }

        inline ast::NodePtr<> default_value() const {
            return m_default;
        }

        /// \brief Returns a string representation of the declaration.
        /// \return a string representation of the declaration
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
    /// \param name the algorithm's name
    /// \param type the algorithm's type
    /// \param desc a brief documentaton of the algorithm
    inline AlgorithmDecl(
        const std::string& name,
        const std::string& type,
        const std::string& desc = "")
        : m_name(name), m_type(type), m_desc(desc) {
    }

    /// \brief Copy constructor.
    /// \param other the object to copy
    inline AlgorithmDecl(const AlgorithmDecl& other)
        : m_name(other.m_name),
          m_type(other.m_type),
          m_desc(other.m_desc),
          m_params(other.m_params) {
    }

    /// \brief Move constructor.
    /// \param other the object to move
    inline AlgorithmDecl(AlgorithmDecl&& other)
        : m_name(std::move(other.m_name)),
          m_type(std::move(other.m_type)),
          m_desc(std::move(other.m_desc)),
          m_params(std::move(other.m_params)) {
    }

    /// \brief Adds a parameter to the declaration.
    /// \param p the parameter to add
    inline void add_param(Param&& p) {
        m_params.emplace_back(std::move(p));
    }

    inline const std::string& name() const { return m_name; }
    inline const std::string& type() const { return m_type; }
    inline const std::string& desc() const { return m_desc; }
    inline const std::vector<Param>& params() const {
        return m_params;
    }

    /// \brief Returns a string representation of the declaration.
    /// \return a string representation of the declaration
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

/// \brief Maps algorithm names to their declarations.
using AlgorithmLib = std::unordered_map<std::string, AlgorithmDecl>;

}} //ns
