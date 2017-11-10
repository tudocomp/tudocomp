#pragma once

#include <tudocomp/pre_header/Env.hpp>

namespace tdc {

inline AlgorithmValue& EnvRoot::algo_value() {
    return *m_algo_value;
}

inline Env::Env(Env&& other):
    m_root(std::move(other.m_root)),
    m_node(other.m_node) {}

inline Env::Env(std::shared_ptr<EnvRoot> root,
                const AlgorithmValue& node):
    m_root(root),
    m_node(node) {}

inline Env::~Env() = default;

inline const AlgorithmValue& Env::algo() const {
    return m_node;
}

inline const std::shared_ptr<EnvRoot>& Env::root() const {
    return m_root;
}

inline void Env::error(const std::string& msg) const {
    throw std::runtime_error(msg);
}

inline Env Env::env_for_option(const std::string& option) const {
    CHECK(algo().arguments().count(option) > 0)
        << "env_for_option(): There is no option '"
        << option
        << "'";
    auto& a = algo().arguments().at(option).as_algorithm();

    return Env(m_root, a);
}

inline const OptionValue& Env::option(const std::string& option) const {
    return algo().arguments().at(option);
}

}

