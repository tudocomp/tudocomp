#ifndef _INCLUDED_CREATE_ALGO_HPP_
#define _INCLUDED_CREATE_ALGO_HPP_

#include <tudocomp/Env.hpp>
#include <tudocomp/Registry.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tudocomp {

/// \cond INTERNAL
inline std::unique_ptr<Compressor> create_algo_with_registry_dynamic(
        const Registry& registry,
        const AlgorithmValue& algorithm_value) {
    return registry.select_algorithm_or_exit(algorithm_value);
}

template<class T, class... Args>
T create_algo_with_registry(const std::string& options,
                            const Registry& registry,
                            Args&&... args) {
    Meta meta = T::meta();
    auto fixed_static_args = std::move(meta).build_static_args_ast_value();

    auto padded_options = meta.name() + "(" + options + ")";
    auto meta_type = meta.type();

    eval::AlgorithmTypes types = registry.algorithm_map();
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

    auto evaluated_options = evald_algo.as_algorithm();

    auto env_root = std::make_shared<EnvRoot>(std::move(evaluated_options));
    Env env(env_root, env_root->algo_value(), registry);

    return T(std::move(env), std::forward<Args>(args)...);
}
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
    return create_algo_with_registry<T>(options, Registry(), std::forward<Args>(args)...);
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

}
#endif
