#ifndef _INCLUDED_ALGORITHM_HPP_
#define _INCLUDED_ALGORITHM_HPP_

#include <tudocomp/AlgorithmStringParser.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <functional>
#include <memory>

namespace tudocomp {

/// \brief Interface for algorithms.
///
/// This is the base for classes that use an environment (\ref Env) to receive
/// options or communicate with the framework in different ways.
///
/// Algorithms are required to implement also a static function \c meta() that
/// returns a Meta object, containing information about the algorithm.
class Algorithm {
    Env m_env;
public:
    /// \brief Default constructor (deleted).
    inline Algorithm() = delete;

    /// \brief Instantiates an algorithm in the specified environment.
    ///
    /// \param env The environment for the algorithm to work in.
    inline Algorithm(Env&& env): m_env(std::move(env)) {}

    /// \brief Provides access to the environment that the algorithm works in.
    /// \return The environment that the algorithm works in.
    inline Env& env() { return m_env; }
};

struct Meta;

/// \cond INTERNAL
inline void gather_types(eval::AlgorithmTypes& target, std::vector<Meta>&& metas);
/// \endcond

/// \brief Template for easy algorithm instantiation.
///
/// Using this function allows the quick instantiation of an Algorithm from its
/// type alone. It will create an environment for the algorithm with the given
/// runtime options and return the created algorithm instance.
///
/// \tparam T The Algorithm type.
/// \tparam Args The constructor's argument types (typically inferred).
/// \param options An options string for the created environment.
/// \param args Additional arguments passed to the algorithm's constructor.
/// \return The created algorithm instance.
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

/// \brief Template for easy algorithm instantiation.
///
/// Using this function allows the quick instantiation of an Algorithm from its
/// type alone. It will create an environment for the algorithm with no runtime
/// options passed and using the algorithm's constructor.
///
/// \tparam T The Algorithm type.
/// \return The created algorithm instance.
template<class T>
T create_algo() {
    return create_algo<T>("");
}

/// \brief Provides meta information about an Algorithm.
///
/// Any Algorithm must implement a static function \c meta() that returns an
/// instance of this class, providing meta information about it.
///
/// The information is used for the automatic instantiation methods (e.g.
/// \ref create_algo) and for automatically generated documentation in the
/// driver application.
///
/// It consists of an algorithm \e type, the \e name, additional documentation,
/// accepted environment \e option and allows the definition of a hierarchy of
/// algorithms. For instance, an algorithm may accept other algorithms of a
/// type \c "strategy" as options, allowing for dynamic switching of strategies
/// based on the application. Please refer to the tutorial documents for more
/// information on this topic.
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

    /// \brief Constructs an algorithm meta object with the given information.
    ///
    /// \param type The algorithm type. This algorithm can be passed as an
    ///             option to other algorithms that accept the same type.
    /// \param name The name of the algorithm, used for the auto-generated
    ///             documentation.
    /// \param doc Further documentation text, used for the auto-generated
    ///             documentation.
    inline Meta(const std::string& type,
                const std::string& name,
                const std::string& doc = ""):
        m_type(type),
        m_name(name),
        m_docs(doc),
        m_static_args(ast::Value(std::string(name), {}))
    {
    }

    /// \brief Helper for declaring accepted options in algorithm meta
    ///        information.
    class OptionBuilder {
        Meta& m_meta;
        std::string m_argument_name;
    public:

        /// \brief Constructor. Do not use, construct via \ref Meta::option instead.
        OptionBuilder(Meta& meta, const std::string& argument_name):
            m_meta(meta), m_argument_name(argument_name) {}

        /// \brief Declares that this option accepts values of the specified
        ///        Algorithm type \c T.
        ///
        /// \tparam T The Algorithm type.
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

        /// \brief Declares that this option accepts values of the specified
        ///        Algorithm type \c T. Its meta information is gathered from
        ///        type \c D.
        ///
        /// Note that type \c D is required to implement a static function
        /// called \c meta() that returns a \ref Meta object containing
        /// information about type \c T.
        ///
        /// \tparam T The Algorithm type.
        /// \tparam D The meta information provider type. \e Must implement a
        ///           static function \ref Meta \c meta().
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

        /// \brief Declares that this option accepts values of a simple type
        ///        that can be parsed from a string (e.g. integers).
        ///
        /// The option will have no default value.
        inline void dynamic() {
            m_meta.check_arg(m_argument_name);
            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                false,
                "string"
            ));
        }

        /// \brief Declares that this option accepts values of a simple type
        ///        that can be parsed from a string (e.g. integers).
        ///default_vaue
        ///        default value.
        inline void dynamic(const std::string& default_vaue) {
            m_meta.check_arg(m_argument_name);
            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                false,
                "string",
                ast::Value(std::string(default_vaue))
            ));
        }
    };

    /// \brief Declares an accepted option for this algorithm.
    ///
    /// \param name The option's name.
    inline OptionBuilder option(const std::string& name) {
        return OptionBuilder(*this, name);
    }

    /// \cond INTERNAL

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

    /// \endcond

    /// \brief Returns the algorithm's name.
    /// \return The algorithm's name.
    inline const std::string& name() const {
        return m_name;
    }

    /// \brief Returns the algorithm's type.
    /// \return The algorithm's type.
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
