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
        auto meta = T::meta();

        if(config_str.empty()) {
            // just create an instance with the default config
            return std::make_unique<T>(meta.default_config());
        } else {
            /*
                //FIXME: This does not yet do what one would expect.
                
                config_str is expected to be complete, including the bindings

                Instead, the meta's default binding config should be extended by
                the user config. Based on that config, the instance should
                be created.
            */
            Registry<T> tmp_registry(meta.decl()->type());
            tmp_registry.template register_algorithm<T>();
            return tmp_registry.select(config_str);
        }
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

