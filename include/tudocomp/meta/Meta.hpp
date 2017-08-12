#pragma once

#include <tuple>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/Parser.hpp>

namespace tdc {
namespace meta {

/// \cond INTERNAL
class Meta;

template<typename Algo>
Meta check_algo_type(const std::string& param_name, const TypeDesc& type);

template<typename Tl>
struct gather;

// TODO: use type_list from textds branch
template<typename Head, typename... Tail>
struct gather<std::tuple<Head, Tail...>> {
    static inline void defaults(
        std::shared_ptr<ast::List> list,
        const std::string& param_name,
        const TypeDesc& type) {

        list->add_value(
            check_algo_type<Head>(param_name, type).decl().default_config());
        gather<std::tuple<Tail...>>::defaults(list, param_name, type);
    }

    static inline void signatures(
        std::shared_ptr<ast::List> list,
        const std::string& param_name,
        const TypeDesc& type) {

        list->add_value(
            check_algo_type<Head>(param_name, type).signature());
        gather<std::tuple<Tail...>>::signatures(list, param_name, type);
    }
};

// TODO: use type_list from textds branch
template<>
struct gather<std::tuple<>> {
    static inline void defaults(
        std::shared_ptr<ast::List> list,
        const std::string& param_name,
        const TypeDesc& type) {
        // done
    }

    static inline void signatures(
        std::shared_ptr<ast::List> list,
        const std::string& param_name,
        const TypeDesc& type) {
        // done
    }
};
/// \endcond

/// \brief Provides meta information about an Algorithm.
class Meta {
private:
    AlgorithmDecl m_decl;
    std::shared_ptr<ast::Object> m_sig; // signature of bindings

public:
    template<typename D>    struct Default {};
    template<typename... D> struct Defaults {};

    class ParamBuilder {
    private:
        Meta* m_meta;
        std::string m_name;

    public:
        inline ParamBuilder(Meta& meta, const std::string& name)
            : m_meta(&meta), m_name(name) {
        }

        inline void primitive() {
            m_meta->m_decl.add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                false,      // no list
                no_type,
                ast::NodePtr<>())); // no default
        }

        template<typename T>
        inline void primitive(const T& default_value) {
            m_meta->m_decl.add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                false,      // no list
                no_type,
                std::make_shared<ast::Value>(to_string(default_value))));
        }

        inline void primitive_list() {
            m_meta->m_decl.add_param(AlgorithmDecl::Param(
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

            m_meta->m_decl.add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                true,       // list
                no_type,
                list));
        }

    private:
        template<typename Binding>
        inline void add_to_signature(const TypeDesc& type) {
            m_meta->m_sig->add_param(ast::Param(
                m_name, check_algo_type<Binding>(m_name, type).signature()));
        }

    public:
        template<typename Binding>
        inline void strategy(const TypeDesc& type) {
            add_to_signature<Binding>(type);
            m_meta->m_decl.add_param(AlgorithmDecl::Param(
                m_name,
                false, // primitive
                false, // no list
                type,
                ast::NodePtr<>() //no default
            ));
        }

        template<typename Binding, typename D>
        inline void strategy(const TypeDesc& type, Meta::Default<D>&&) {
            add_to_signature<Binding>(type);
            auto def =
                check_algo_type<D>(m_name, type).decl().default_config();

            m_meta->m_decl.add_param(AlgorithmDecl::Param(
                m_name,
                false, // non-primitive
                false, // no list
                type,
                def));
        }

    private:
        template<typename... Bindings>
        inline void add_list_to_signature(const TypeDesc& type) {
            auto sigs = std::make_shared<ast::List>();
            gather<std::tuple<Bindings...>>::signatures(sigs, m_name, type);
            m_meta->m_sig->add_param(ast::Param(m_name, sigs));
        }

    public:
        template<typename... Bindings>
        inline void strategy_list(const TypeDesc& type) {
            add_list_to_signature<Bindings...>(type);

            m_meta->m_decl.add_param(AlgorithmDecl::Param(
                m_name,
                false, // non-primitive
                true,  // list
                type,
                ast::NodePtr<>() //no default
            ));
        }

        template<typename... Bindings, typename... D>
        inline void strategy_list(
            const TypeDesc& type,
            Meta::Defaults<D...>&&) {

            add_list_to_signature<Bindings...>(type);

            auto defaults = std::make_shared<ast::List>();
            gather<std::tuple<D...>>::defaults(defaults, m_name, type);

            m_meta->m_decl.add_param(AlgorithmDecl::Param(
                m_name,
                false, // non-primitive
                true,  // list
                type,
                defaults
            ));
        }
    };

    inline Meta(
        const std::string& name,
        const TypeDesc&    type,
        const std::string& desc)
        : m_decl(AlgorithmDecl(name, type, desc)),
          m_sig(std::make_shared<ast::Object>(name)) {
    }

    inline ParamBuilder param(const std::string& name) {
        return ParamBuilder(*this, name);
    }

    inline const AlgorithmDecl& decl() const {
        return m_decl;
    }

    inline ast::NodePtr<ast::Object> signature() const {
        return m_sig;
    }
};

/// \cond INTERNAL
template<typename Algo>
inline Meta check_algo_type(
    const std::string& param_name,
    const TypeDesc& type) {

    auto meta = Algo::meta();
    auto& decl = meta.decl();
    if(!decl.type().subtype_of(type)) {
        throw DeclError(std::string(
            "algorithm type mismatch in default value for parameter '") +
            param_name + "': expected " + type.name() + ", got " +
            decl.type().name() + " ('" +
            decl.name() + "')");
    }

    return meta;
}
/// \endcond

} //namespace meta

using Meta = meta::Meta;

} //namespace tdc

