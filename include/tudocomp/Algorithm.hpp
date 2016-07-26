#ifndef _INCLUDED_ALGORITHM_HPP_
#define _INCLUDED_ALGORITHM_HPP_

#include <tudocomp/AlgorithmStringParser.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <functional>
#include <memory>

namespace tudocomp {

/// Interface for a general algorithm.
class Algorithm {
    Env m_env;
public:
    /// Class needs to be constructed with an `Env&` argument.
    inline Algorithm() = delete;

    /// Construct the class with an environment.
    inline Algorithm(Env&& env): m_env(std::move(env)) {}

    /// Return a reference to the environment
    inline Env& env() { return m_env; }
};

struct Meta;
inline void gather_types(eval::AlgorithmTypes& target, std::vector<Meta>&& metas);

/// meta: the AlgorithmMeta of the algorithm to create
/// options: optional options to set / overwrite from default
template<class T, class... Args>
T create_algo(const std::string& options, Args&&... args) {
    auto meta = T::meta();
    auto fixed_static_args = std::move(meta).build_static_args_ast_value();

    auto padded_options = meta.name() + "(" + options + ")";
    auto meta_type = meta.type();

    eval::AlgorithmTypes types;
    gather_types(types, {
        std::move(meta)
    });

    ast::Parser p { padded_options };

    auto evald_algo = eval::cl_eval(
        p.parse_value(),
        meta_type,
        types,
        std::move(fixed_static_args)
    );

    auto& evaluated_options = evald_algo.options.as_algorithm();

    auto env_root = std::make_shared<EnvRoot>(std::move(evaluated_options));
    Env env(env_root, env_root->algo_value());

    return T(std::move(env), std::forward<Args>(args)...);
}

/// meta: the AlgorithmMeta of the algorithm to create
/// options: optional options to set / overwrite from default
template<class T>
T create_algo() {
    return create_algo<T>("");
}

class Meta {
    std::string m_type;
    std::string m_name;
    std::string m_docs;

    std::vector<decl::Arg> m_options;

    ast::Value m_static_args;

    std::vector<Meta> m_sub_metas;

    inline void check_arg(const std::string& argument_name) {
        for (auto& e : m_options) {
            if (e.name() == argument_name) {
                throw std::runtime_error("argument already defined");
            }
        }
    }

    friend inline void gather_types(eval::AlgorithmTypes& target,
                                    Meta&& meta);
public:

    inline Meta(const std::string& type,
                const std::string& name,
                const std::string& doc = ""):
        m_type(type),
        m_name(name),
        m_docs(doc),
        m_static_args(ast::Value(std::string(name), {}))
    {
    }

    class OptionBuilder {
        Meta& m_meta;
        std::string m_argument_name;
    public:
        OptionBuilder(Meta& meta, const std::string& argument_name):
            m_meta(meta), m_argument_name(argument_name) {}

        struct NoDefault {};

        template<class T>
        inline void templated() {
            m_meta.check_arg(m_argument_name);
            Meta sub_meta = T::meta();
            m_meta.m_sub_metas.push_back(sub_meta);

            auto type = std::move(sub_meta.m_type);

            m_meta.m_static_args.invokation_arguments().push_back(ast::Arg(
                std::string(m_argument_name),
                std::move(sub_meta).build_static_args_ast_value()
            ));

            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                true,
                std::move(type)
            ));
        }

        template<class T, class D>
        inline void templated() {
            templated<T>();
            m_meta.m_options.pop_back();

            Meta sub_meta = D::meta();
            m_meta.m_sub_metas.push_back(sub_meta);

            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                true,
                std::move(sub_meta.m_type),
                std::move(sub_meta).build_ast_value()
            ));
        }

        inline void dynamic() {
            m_meta.check_arg(m_argument_name);
            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                false,
                "string"
            ));
        }

        inline void dynamic(const std::string& argument_default) {
            m_meta.check_arg(m_argument_name);
            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                false,
                "string",
                ast::Value(std::string(argument_default))
            ));
        }
    };

    inline OptionBuilder option(const std::string& argument_name) {
        return OptionBuilder(*this, argument_name);
    }

    inline decl::Algorithm build_def() && {
        return decl::Algorithm(
            std::move(m_name),
            std::move(m_options),
            std::move(m_docs)
        );
    }

    inline ast::Value build_ast_value() && {
        std::vector<ast::Arg> options;
        for (auto& arg : m_options) {
            options.push_back(ast::Arg(
                arg.name(),
                arg.is_static(),
                arg.type(),
                std::move(arg.default_value())
            ));
        }

        return ast::Value(std::move(m_name), std::move(options));
    }

    inline ast::Value build_static_args_ast_value() && {
        return std::move(m_static_args);
    }

    inline const std::string& name() const {
        return m_name;
    }

    inline const std::string& type() const {
        return m_type;
    }

};

inline void gather_types(eval::AlgorithmTypes& target,
                         Meta&& meta)
{
    auto& target_type_algos = target[meta.type()];

    // Create decl::Algorithm value here

    auto decl_value = std::move(meta).build_def();

    bool found = false;
    for (auto& already_seen_algo : target_type_algos) {
        if (already_seen_algo.name() == decl_value.name()) {
            // TODO: Nice error
            CHECK(already_seen_algo == decl_value);

            found = true;
            break;
        }
    }

    if (!found) {
        target_type_algos.push_back(std::move(decl_value));
    }

    gather_types(target, std::move(meta.m_sub_metas));
}

inline void gather_types(eval::AlgorithmTypes& target,
                         std::vector<Meta>&& metas)
{
    for (auto& meta : metas) {
        gather_types(target, std::move(meta));
    }
}

}

#endif
