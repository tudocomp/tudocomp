#pragma once

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/Parser.hpp>

namespace tdc {
namespace meta {

/// \brief Provides meta information about an Algorithm.
class Meta {
private:
    meta::AlgorithmDecl m_decl;

public:
    inline Meta(
        const std::string& name,
        const std::string& type,
        const std::string& desc)
        : m_decl(meta::AlgorithmDecl(name, type, desc)) {
    }

    inline void param(
        const std::string& name,
        const std::string& default_value = "") {
        
        m_decl.add_param(meta::AlgorithmDecl::Param(
            name,
            true,  // primitive
            false, // no list
            "",    // no type
            default_value.empty() ?
                ast::NodePtr<>() :
                ast::Parser::parse(default_value)));
    }

    inline void param_list(
        const std::string& name,
        const std::string& default_value = "") {
        
        m_decl.add_param(meta::AlgorithmDecl::Param(
            name,
            true, // primitive
            true, // list
            "",   // no type
            default_value.empty() ?
                ast::NodePtr<>() :
                ast::Parser::parse(default_value)));
    }

    inline void sub_algo(
        const std::string& param_name,
        const std::string& type) {

        m_decl.add_param(meta::AlgorithmDecl::Param(
            param_name,
            false, // primitive
            false, // no list
            type,  // no type
            ast::NodePtr<>())); // TODO: default?
    }

    inline const meta::AlgorithmDecl& decl() const {
        return m_decl;
    }
};

} //namespace meta

using Meta = meta::Meta;

} //namespace tdc

