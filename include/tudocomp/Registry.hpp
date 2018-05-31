#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <istream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <glog/logging.h>

#include <tudocomp/def.hpp>
#include <tudocomp/pre_header/RegistryOf.hpp>

namespace tdc {

class Registry {
private:
    using map_t = typename WeakRegistry::map_t;
    std::shared_ptr<map_t> m_registries;

    friend class WeakRegistry;

    inline Registry(std::shared_ptr<map_t> r): m_registries(r) {}
public:
    inline Registry(): m_registries(std::make_shared<map_t>()) {}

    inline AlgorithmValue& algo_value();
    template<typename T>
    inline void register_registry(RegistryOf<T> const& registry) {
        registry.m_data->m_registry = *this;
        m_registries->insert(std::make_pair(
            std::string(registry.root_type()),
            std::make_unique<RegistryOf<T>>(registry)
        ));
    }

    template<typename algorithm_if_t>
    inline RegistryOf<algorithm_if_t> const& of() const {
        auto a = algorithm_if_t::meta_type();

        if (m_registries->count(a) == 0) {
            m_registries->insert(std::make_pair(
                std::string(a),
                std::make_unique<RegistryOf<algorithm_if_t>>(*this)
            ));
        }

        return m_registries->at(a)->template downcast<algorithm_if_t>();
    }

    template<typename algorithm_if_t>
    inline RegistryOf<algorithm_if_t>& of() {
        auto a = algorithm_if_t::meta_type();

        if (m_registries->count(a) == 0) {
            m_registries->insert(std::make_pair(
                std::string(a),
                std::make_unique<RegistryOf<algorithm_if_t>>(*this)
            ));
        }

        return m_registries->at(a)->template downcast<algorithm_if_t>();
    }
};

inline WeakRegistry::WeakRegistry(Registry const& r):
    m_registries(r.m_registries) {}

inline WeakRegistry::operator Registry() const {
    DCHECK(!m_registries.expired());
    return m_registries.lock();
}
inline WeakRegistry::WeakRegistry() = default;
inline WeakRegistry::~WeakRegistry() = default;

}
