#pragma once

#include <tuple>
#include <vector>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/Parser.hpp>

#include <tudocomp/io/InputRestrictions.hpp>

namespace tdc {
namespace meta {

/// \cond INTERNAL
class Meta;

template<typename Algo>
Meta check_algo_type(const std::string& param_name, const TypeDesc& type);

void add_to_lib(AlgorithmLib& target, const Meta& meta);

template<typename Tl>
struct gather;

// TODO: use type_list from textds branch
template<typename Head, typename... Tail>
struct gather<std::tuple<Head, Tail...>> {
    static inline void metas(
        std::vector<Meta>& out,
        const std::string& param_name,
        const TypeDesc& type) {

        out.push_back(check_algo_type<Head>(param_name, type));
        gather<std::tuple<Tail...>>::metas(out, param_name, type);
    }
};

// TODO: use type_list from textds branch
template<>
struct gather<std::tuple<>> {
    static inline void metas(
        std::vector<Meta>& out,
        const std::string& param_name,
        const TypeDesc& type) {

        // done
    }
};
/// \endcond

/// \brief Provides meta information about an Algorithm.
class Meta {
private:
    std::shared_ptr<AlgorithmDecl> m_decl;
    std::shared_ptr<ast::Object> m_sig; // signature of bindings
    AlgorithmLib m_known; // library of known declarations (excluding self!)

    // further meta information
    // TODO: generalize?
    InputRestrictions m_input_restrictions;

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
            m_meta->m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                false,      // no list
                no_type,
                ast::NodePtr<>())); // no default
        }

        template<typename T>
        inline void primitive(const T& default_value) {
            m_meta->m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                false,      // no list
                no_type,
                std::make_shared<ast::Value>(to_string(default_value))));
        }

        inline void primitive_list() {
            m_meta->m_decl->add_param(AlgorithmDecl::Param(
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

            m_meta->m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                true,       // primitive
                true,       // list
                no_type,
                list));
        }

    private:
        template<typename Binding>
        inline Meta register_binding(const TypeDesc& type) {
            auto meta = check_algo_type<Binding>(m_name, type);
            m_meta->m_sig->add_param(ast::Param(m_name, meta.signature()));
            add_to_lib(m_meta->m_known, meta);
            return meta;
        }

    public:
        template<typename Binding>
        inline void strategy(const TypeDesc& type) {
            register_binding<Binding>(type);

            m_meta->m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                false, // primitive
                false, // no list
                type,
                ast::NodePtr<>() //no default
            ));
        }

        template<typename Binding, typename D>
        inline void strategy(const TypeDesc& type, Meta::Default<D>&&) {
            register_binding<Binding>(type);

            m_meta->m_decl->add_param(AlgorithmDecl::Param(
                m_name,
                false, // non-primitive
                false, // no list
                type,
                D::meta().decl()->default_config()));
        }

    private:
        template<typename... Bindings>
        inline std::vector<Meta> register_bindings(const TypeDesc& type) {
            std::vector<Meta> metas;
            gather<std::tuple<Bindings...>>::metas(metas, m_name, type);

            auto sigs = std::make_shared<ast::List>();
            for(auto& meta : metas) {
                sigs->add_value(meta.signature());
                add_to_lib(m_meta->m_known, meta);
            }

            m_meta->m_sig->add_param(ast::Param(m_name, sigs));
            return metas;
        }

    public:
        template<typename... Bindings>
        inline void strategy_list(const TypeDesc& type) {
            register_bindings<Bindings...>(type);

            m_meta->m_decl->add_param(AlgorithmDecl::Param(
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

            register_bindings<Bindings...>(type);

            auto defaults = std::make_shared<ast::List>();
            {
                std::vector<Meta> metas;
                gather<std::tuple<D...>>::metas(metas, m_name, type);

                for(auto& meta : metas) {
                    defaults->add_value(meta.decl()->default_config());
                }
            }

            m_meta->m_decl->add_param(AlgorithmDecl::Param(
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
        : m_decl(std::make_shared<AlgorithmDecl>(name, type, desc)),
          m_sig(std::make_shared<ast::Object>(name)) {
    }

    inline ParamBuilder param(const std::string& name) {
        return ParamBuilder(*this, name);
    }

    inline std::shared_ptr<const AlgorithmDecl> decl() const {
        return m_decl;
    }

    inline ast::NodePtr<ast::Object> signature() const {
        return m_sig;
    }

    inline const AlgorithmLib& known() const {
        return m_known;
    }

    inline const InputRestrictions& input_restrictions() const {
        return m_input_restrictions;
    }

    inline void input_restrictions(InputRestrictions r) {
        m_input_restrictions = r;
    }
};

/// \cond INTERNAL
template<typename Algo>
inline Meta check_algo_type(
    const std::string& param_name,
    const TypeDesc& type) {

    auto meta = Algo::meta();
    auto decl = meta.decl();
    if(!decl->type().subtype_of(type)) {
        throw DeclError(std::string(
            "algorithm type mismatch in default value for parameter '") +
            param_name + "': expected " + type.name() + ", got " +
            decl->type().name() + " ('" +
            decl->name() + "')");
    }

    return meta;
}

inline void add_to_lib(AlgorithmLib& target, const Meta& meta) {
    for(auto e : meta.known()) add_to_lib(target, e.second);
    add_to_lib(target, meta.decl());
}
/// \endcond

} //namespace meta

using Meta = meta::Meta;

} //namespace tdc

