#pragma once

#include <tudocomp/meta/RegistryOf.hpp>

namespace tdc {
namespace meta {

class Registry {
private:
    static Registry global;

    using regmap_t = std::unordered_map<
        TypeDesc, std::shared_ptr<RegistryOfAny>>;

    regmap_t m_regs;

public:
    template<typename T>
    static inline RegistryOf<T>& of() {
        return global.get<T>();
    }

    inline Registry() {
    }

    /// \cond INTERNAL
    template<typename T>
    inline RegistryOf<T>& get() {
        auto type = T::type_desc();

        auto it = m_regs.find(type);
        if(it != m_regs.end()) {
            // registry already exists
            return *(std::static_pointer_cast<RegistryOf<T>>(it->second));
        } else {
            // create new registry
            auto newreg = std::make_shared<RegistryOf<T>>();
            m_regs.emplace(type, newreg);
            return *newreg;
        }
    }
    /// \endcond INTERNAL
};

} //namespace meta

using Registry = meta::Registry;

} //namespace tdc
