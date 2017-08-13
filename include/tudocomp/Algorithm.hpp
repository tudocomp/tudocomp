#pragma once

#include <tudocomp/meta/AlgorithmConfig.hpp>
#include <tudocomp/meta/Meta.hpp>
#include <tudocomp/meta/Registry.hpp>

namespace tdc {

class Algorithm {
    AlgorithmConfig m_config;

public:
    template<typename T>
    static inline std::unique_ptr<T> instance(std::string config = "") {
        Registry<T> tmp_registry;
        tmp_registry.template register_algorithm<T>();

        if(config.empty()) {
            //FIXME: bindings may be different from default config!
            config = T::meta().decl()->default_config()->str();
        }

        return tmp_registry.select(config);
    }

    inline Algorithm() = delete;
    inline Algorithm(AlgorithmConfig&& cfg): m_config(std::move(cfg)) {}

    inline const AlgorithmConfig& config() const { return m_config; }
};

}

