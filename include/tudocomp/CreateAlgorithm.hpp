#pragma once

#include <tudocomp/Env.hpp>
#include <tudocomp/Registry.hpp>
#include <tudocomp/RegistryOf.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

namespace tdc {

template<typename T, typename registry_root_t = Compressor>
class Builder {
    Registry m_registry;
    std::string m_options;
public:
    inline Builder() {}

    /// Sets an options string for the created environment.
    inline Builder& options(const std::string& options) {
        m_options = options;
        return *this;
    }


    /// Sets a registry to be used for dynamic type lookups during instantiation.
    inline Builder& registry(const Registry& reg) {
        m_registry = reg;
        return *this;
    }

    /// Creates the `Env` instance needed for instantiating `T`.
    inline Env env() {
        Meta meta = T::meta();
        auto fixed_static_args = std::move(meta).build_static_args_ast_value();

        auto padded_options = meta.name() + "(" + m_options + ")";
        auto meta_type = meta.type();

        eval::AlgorithmTypes types = m_registry.registry<registry_root_t>().algorithm_map();
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

        auto env_root = EnvRoot(m_registry, std::move(evaluated_options));
        Env env(env_root, env_root.algo_value());

        return env;
    }

    /// Instances `T` by creating the right `Env` instance, and
    /// passing any extra arguments to the constructor of `T`.
    ///
    /// \tparam Args The constructor's argument types (typically inferred).
    /// \param args Additional arguments passed to the algorithm's constructor.
    template<typename... Args>
    inline T instance(Args&&... args) {
        return T(env(), std::forward<Args>(args)...);
    }
};

/// \brief Builder pattern template for easy algorithm instantiation.
///
/// Using this function allows the quick instantiation of an Algorithm from its
/// type alone. It will create an environment for the algorithm with the given
/// runtime options and return the created algorithm instance.
///
/// Because it has a few optional arguments this function returns a builder
/// for the actual algorithm instance.
///
/// \tparam T The Algorithm type.
/// \return The algorithm builder instance.
template<typename T, typename registry_root_t = Compressor>
Builder<T, registry_root_t> builder() {
    return Builder<T, registry_root_t>();
}

/// \cond INTERNAL
template<typename T, typename registry_root_t, typename... Args>
T create_algo_with_registry(const std::string& options,
                            const Registry& registry,
                            Args&&... args) {
    return builder<T, registry_root_t>()
        .registry(registry)
        .options(options)
        .instance(std::forward<Args>(args)...);
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
    Registry registry;

    return create_algo_with_registry<T, Compressor, Args...>(options, registry, std::forward<Args>(args)...);
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

/// \brief Creates an environment.
///
/// The environment will be created based on arbitrary \ref tdc::Meta
/// information of an algorithm that may or not really exist. No registry
/// is used to verify.
///
/// \param meta the meta information to use to create the environment.
/// \param options algorithm options to set in the environment.
/// \return the newly created environment.
inline Env create_env(Meta&& meta, const std::string& options = "") {
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

    Registry regregs;

    auto evaluated_options = evald_algo.as_algorithm();
    auto env_root = EnvRoot(regregs, std::move(evaluated_options));
    Env env(env_root, env_root.algo_value());

    return env;
}

}

