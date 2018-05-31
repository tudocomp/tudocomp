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
#include <tudocomp/OptionValue.hpp>
#include <tudocomp/pre_header/Registry.hpp>

namespace tdc {

class EnvRoot: std::enable_shared_from_this<EnvRoot> {
private:
    std::unique_ptr<AlgorithmValue> m_algo_value;
    std::unordered_map<std::string, std::unique_ptr<VirtualRegistry>> m_registries;

public:
    inline EnvRoot() = default;

    inline EnvRoot(AlgorithmValue&& algo_value):
        m_algo_value(std::make_unique<AlgorithmValue>(std::move(algo_value)))  {
    }

    inline AlgorithmValue& algo_value();
    template<typename T>
    inline void register_registry(Registry<T> const& registry) {
        m_registries.insert(std::make_pair(
            std::string(registry.root_type()),
            std::make_unique<Registry<T>>(registry)
        ));
    }

    template<typename algorithm_if_t>
    inline Registry<algorithm_if_t> const& registry() const {
        auto a = algorithm_if_t::meta_type();
        DLOG(INFO) << "registry lookup of " << "'" << a << "'";
        DLOG(INFO) << "known entries {";
        for (auto const& kv : m_registries) {
            DLOG(INFO) << "    " << kv.first;
        }
        DLOG(INFO) << "}";

        return m_registries.at(a)->template downcast<algorithm_if_t>();
    }

    /// \cond INTERNAL
    template<typename algorithm_if_t>
    inline std::unique_ptr<algorithm_if_t> select_algorithm(AlgorithmValue const& algo) {
        std::shared_ptr<EnvRoot> shared = shared_from_this();
        return registry<algorithm_if_t>().select_algorithm(shared, algo);
    }
    /// \endcond
};

}
