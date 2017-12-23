#pragma once

#include <tudocomp/meta/AlgorithmConfig.hpp>
#include <tudocomp/meta/Meta.hpp>

namespace tdc {

class Algorithm {
    AlgorithmConfig m_config;

public:
    template<typename T, typename... Args>
    static inline T instance(std::string config_str = "", Args&&... args) {
        using namespace meta;

        auto meta = T::meta();

        if(config_str.empty()) {
            // just create an instance with the default config
            return T(meta.default_config(), args...);
        } else {
            // create an instance with an override config
            return T(meta.default_config(
                ast::convert<ast::Object>(
                    ast::Parser::parse(
                        meta.decl()->name() + paranthesize(config_str)))),
                std::forward<Args>(args)...);
        }
    }

    template<typename T, typename... Args>
    static inline T instance(const char* config_str, Args&&... args) {
        return instance<T>(std::string(config_str),
            std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static inline T instance(Args&&... args) {
        return instance<T>(std::string(),
            std::forward<Args>(args)...);
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

