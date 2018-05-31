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
#include <tudocomp/pre_header/RegistryOf.hpp>
#include <tudocomp/Registry.hpp>

namespace tdc {

class EnvRoot {
private:
    struct this_t {
        std::unique_ptr<AlgorithmValue> m_algo_value;
        Registry m_registries;
    };
    std::shared_ptr<this_t> m_this;

public:
    inline EnvRoot(): m_this(std::make_shared<this_t>()) {}

    inline EnvRoot(Registry const& regreg, AlgorithmValue&& algo_value):
        m_this(std::make_shared<this_t>(this_t {
            std::make_unique<AlgorithmValue>(std::move(algo_value)),
            std::move(regreg),
        })) {}

    inline AlgorithmValue& algo_value() {
        return *m_this->m_algo_value;
    }

    template<typename algorithm_if_t>
    inline RegistryOf<algorithm_if_t> const& registry() const {
        return m_this->m_registries.of<algorithm_if_t>();
    }

    /// \cond INTERNAL
    template<typename algorithm_if_t>
    inline std::unique_ptr<algorithm_if_t> select_algorithm(AlgorithmValue const& algo) const {
        EnvRoot shared = *this;;
        return registry<algorithm_if_t>().select_algorithm(shared, algo);
    }
    /// \endcond
};

}
