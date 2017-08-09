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
        const TypeDesc&    type,
        const std::string& desc)
        : m_decl(AlgorithmDecl(name, type, desc)) {
    }

    inline void param(
        const std::string& name,
        const std::string& default_value = "") {

        m_decl.add_param(AlgorithmDecl::Param(
            name,
            true,       // primitive
            false,      // no list
            TypeDesc(), // no type
            default_value.empty() ?
                ast::NodePtr<>() :
                ast::Parser::parse(default_value)));
    }

    inline void param_list(
        const std::string& name,
        const std::string& default_value = "") {

        m_decl.add_param(AlgorithmDecl::Param(
            name,
            true,       // primitive
            true,       // list
            TypeDesc(), // no type
            default_value.empty() ?
                ast::NodePtr<>() :
                ast::Parser::parse(default_value)));
    }

    inline void sub_algo(
        const std::string& param_name,
        const TypeDesc& type) {

        m_decl.add_param(AlgorithmDecl::Param(
            param_name,
            false, // primitive
            false, // no list
            type,
            ast::NodePtr<>() //no default
        ));
    }

    template<typename Default>
    inline void sub_algo(
        const std::string& param_name,
        const TypeDesc& type) {

        auto default_decl = Default::meta().decl();

        if(!default_decl.type().subtype_of(type)) {
            throw DeclError(
                std::string("type mismatch in default value for parameter '") +
                    param_name + "': expected " + type.name() + ", got " +
                    default_decl.type().name() + " ('" +
                    default_decl.name() + "')");
        }

        m_decl.add_param(AlgorithmDecl::Param(
            param_name,
            false, // primitive
            false, // no list
            type,  // no type
            default_decl.default_config()));
    }

    inline const AlgorithmDecl& decl() const {
        return m_decl;
    }
};

} //namespace meta

using Meta = meta::Meta;

} //namespace tdc

