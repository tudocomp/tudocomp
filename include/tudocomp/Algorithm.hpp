#pragma once

#include <memory>

#include <tudocomp/meta/Config.hpp>
#include <tudocomp/meta/Meta.hpp>

namespace tdc {

class Algorithm {
    Config m_config;

public:
    template<typename T, typename... Args>
    static inline std::unique_ptr<T> instance(
        std::string config_str,
        Meta meta,
        Args&&... args) {

        return std::make_unique<T>(meta.config(config_str),
            std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static inline std::unique_ptr<T> instance(
        std::string config_str,
        Args&&... args) {

        return instance<T>(config_str,
            T::meta(),
            std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static inline std::unique_ptr<T> instance(
        const char* config_str,
        Args&&... args) {

        return instance<T>(std::string(config_str),
            T::meta(),
            std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static inline std::unique_ptr<T> instance(Meta meta, Args&&... args) {
        return instance<T>(std::string(),
            meta,
            std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static inline std::unique_ptr<T> instance(Args&&... args) {
        return instance<T>(std::string(),
            T::meta(),
            std::forward<Args>(args)...);
    }

    template<typename T>
    static inline std::unique_ptr<T> instance(Meta meta) {
        return instance<T>(std::string(), meta);
    }

    template<typename T>
    static inline std::unique_ptr<T> instance() {
        return instance<T>(std::string(), T::meta());
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

