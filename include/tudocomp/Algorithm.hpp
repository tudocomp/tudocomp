#pragma once

#include <tudocomp/meta/AlgorithmConfig.hpp>
#include <tudocomp/meta/Meta.hpp>
#include <tudocomp/meta/Registry.hpp>

namespace tdc {

class Algorithm {
    AlgorithmConfig m_config;

public:
    template<typename T>
    static inline std::unique_ptr<T> instance(std::string config_str = "") {
        Registry<T> tmp_registry;
        tmp_registry.template register_algorithm<T>();

        //FIXME: bindings may be different from default config values!
        auto ast = config_str.empty() ?
            T::meta().decl()->default_config() :
            meta::ast::Parser::parse(config_str);

        return tmp_registry.select(ast);
    }

    inline Algorithm() = delete;
    inline Algorithm(AlgorithmConfig&& cfg): m_config(std::move(cfg)) {}

    inline const AlgorithmConfig& config() const { return m_config; }

    [[deprecated("use config()")]]
    inline const AlgorithmConfig& env() const { return m_config; }

    [[deprecated("transitional solution")]]
    inline AlgorithmConfig& env() { return m_config; }
};

}

