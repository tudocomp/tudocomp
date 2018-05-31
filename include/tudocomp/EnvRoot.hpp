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
#include <tudocomp/RegistryRegistry.hpp>
#include <tudocomp/Registry.hpp>

namespace tdc {

class EnvRoot: std::enable_shared_from_this<EnvRoot> {
private:
    std::unique_ptr<AlgorithmValue> m_algo_value;
    RegistryRegistry m_registries;

public:
    inline EnvRoot() = default;

    inline EnvRoot(RegistryRegistry const& regreg, AlgorithmValue&& algo_value):
        m_algo_value(std::make_unique<AlgorithmValue>(std::move(algo_value))),
        m_registries(std::move(regreg)) {
    }

    inline AlgorithmValue& algo_value() {
        return *m_algo_value;
    }

    template<typename algorithm_if_t>
    inline Registry<algorithm_if_t> const& registry() const {
        return m_registries.registry<algorithm_if_t>();
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
