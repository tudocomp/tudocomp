#pragma once

#include <tuple>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/Parser.hpp>

namespace tdc {
namespace meta {

/// \cond INTERNAL
template<typename Algo>
inline const AlgorithmDecl& check_algo_type(
    const std::string& param_name,
    const TypeDesc& type) {

    auto& decl = Algo::meta().decl();
    if(!decl.type().subtype_of(type)) {
        throw DeclError(std::string(
            "type mismatch in default value for parameter '") +
            param_name + "': expected " + type.name() + ", got " +
            decl.type().name() + " ('" +
            decl.name() + "')");
    }

    return decl;
}

template<typename Tl>
struct gather_defaults;

// TODO: use type_list from textds branch
template<typename Head, typename... Tail>
struct gather_defaults<std::tuple<Head, Tail...>> {
    static inline std::shared_ptr<ast::List> list(
        const std::string& param_name,
        const TypeDesc& type) {

        auto list =
            gather_defaults<std::tuple<Tail...>>::list(param_name, type);

        auto& decl = check_algo_type<Head>(param_name, type);
        list->add_value(decl.default_config());
        return list;
    }
};

// TODO: use type_list from textds branch
template<>
struct gather_defaults<std::tuple<>> {
    static inline std::shared_ptr<ast::List> list(
        const std::string& param_name,
        const TypeDesc& type) {

        return std::make_shared<ast::List>(); // base
    }
};
/// \endcond

/// \brief Provides meta information about an Algorithm.
class Meta {
private:
    AlgorithmDecl m_decl;

public:
    class ParamBuilder {
    private:
        AlgorithmDecl* m_decl;
        std::string m_name;

    public:
        inline ParamBuilder(AlgorithmDecl& decl, const std::string& name)
            : m_decl(&decl), m_name(name) {
        }

        inline void primitive() {
            m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                false,      // no list
                no_type,
                ast::NodePtr<>())); // no default
        }

        template<typename T>
        inline void primitive(const T& default_value) {
            m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                false,      // no list
                no_type,
                std::make_shared<ast::Value>(to_string(default_value))));
        }

        inline void primitive_list() {
            m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                true,       // list
                no_type,
                ast::NodePtr<>())); // no default
        }

        template<typename T>
        inline void primitive_list(std::initializer_list<T> default_value) {
            auto list = std::make_shared<ast::List>();
            for(auto& v : default_value) {
                list->add_value(std::make_shared<ast::Value>(to_string(v)));
            }

            m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                true,       // list
                no_type,
                list));
        }

        inline void strategy(const TypeDesc& type) {
            m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                false, // primitive
                false, // no list
                type,
                ast::NodePtr<>() //no default
            ));
        }

        template<typename Default>
        inline void strategy(const TypeDesc& type) {
            auto& default_decl = check_algo_type<Default>(m_name, type);

            m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                false, // non-primitive
                false, // no list
                type,
                default_decl.default_config()));
        }

        inline void strategy_list(const TypeDesc& type) {
            m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                false, // non-primitive
                true,  // list
                type,
                ast::NodePtr<>() //no default
            ));
        }

        template<typename... Defaults>
        inline void strategy_list(const TypeDesc& type) {
            auto defaults =
                gather_defaults<std::tuple<Defaults...>>::list(m_name, type);

            m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                false, // non-primitive
                true,  // list
                type,
                std::const_pointer_cast<ast::List>(defaults)
            ));
        }
    };

    inline Meta(
        const std::string& name,
        const TypeDesc&    type,
        const std::string& desc)
        : m_decl(AlgorithmDecl(name, type, desc)) {
    }

    inline ParamBuilder param(const std::string& name) {
        return ParamBuilder(m_decl, name);
    }

    inline const AlgorithmDecl& decl() const {
        return m_decl;
    }
};

} //namespace meta

using Meta = meta::Meta;

} //namespace tdc

