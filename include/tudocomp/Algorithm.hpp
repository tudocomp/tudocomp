#pragma once

#include <memory>

#include <tudocomp/meta/Config.hpp>
#include <tudocomp/meta/Meta.hpp>

namespace tdc {

class Algorithm {
    Config m_config;

public:
    template<typename T, typename... Args>
    static inline std::unique_ptr<T> unique_instance(
        std::string config_str, Args&&... args) {

        return std::make_unique<T>(T::meta().config(config_str),
            std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static inline std::unique_ptr<T> unique_instance(
        const char* config_str, Args&&... args) {

        return unique_instance<T>(std::string(config_str),
            std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static inline std::unique_ptr<T> unique_instance(Args&&... args) {
        return unique_instance<T>(std::string(),
            std::forward<Args>(args)...);
    }

    template<typename T>
    static inline std::unique_ptr<T> unique_instance() {
        return unique_instance<T>(std::string());
    }

    template<typename T, typename... Args>
    static inline T instance(std::string config_str, Args&&... args) {
        return T(T::meta().config(config_str),
            std::forward<Args>(args)...);
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

    template<typename T>
    static inline T instance() {
        return instance<T>(std::string());
    }

    inline Algorithm() = delete;
    inline Algorithm(Config&& cfg): m_config(std::move(cfg)) {}

    inline const Config& config() const { return m_config; }

    [[deprecated("transitional alias - use config()")]]
    inline const Config& env() const { return m_config; }

    [[deprecated("transitional alias - use config()")]]
    inline Config& env() { return m_config; }
};

}

