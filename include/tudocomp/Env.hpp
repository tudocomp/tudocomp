#pragma once

#include <tudocomp/pre_header/Registry.hpp>
#include <tudocomp/pre_header/Env.hpp>

namespace tdc {

inline AlgorithmValue& EnvRoot::algo_value() {
    return *m_algo_value;
}

inline Stat& EnvRoot::stat_current() {
    return m_stat_stack.empty() ? m_stat_root : m_stat_stack.top();
}

inline void EnvRoot::begin_stat_phase(const std::string& name) {
    DLOG(INFO) << "begin phase \"" << name << "\"";

    m_stat_stack.push(Stat(name));
    Stat& stat = m_stat_stack.top();
    stat.begin();
}

inline void EnvRoot::end_stat_phase() {
    DCHECK(!m_stat_stack.empty());

    Stat& stat_ref = m_stat_stack.top();
    DLOG(INFO) << "end phase \"" << stat_ref.title() << "\"";

    stat_ref.end();

    Stat stat = stat_ref; //copy
    m_stat_stack.pop();

    if(!m_stat_stack.empty()) {
        Stat& parent = m_stat_stack.top();
        parent.add_sub(stat);
    } else {
        m_stat_root = stat;
    }
}

inline Stat& EnvRoot::finish_stats() {
    while(!m_stat_stack.empty()) {
        end_stat_phase();
    }

    return m_stat_root;
}

inline void EnvRoot::restart_stats(const std::string& root_name) {
    finish_stats();
    begin_stat_phase(root_name);
}

template<class T>
inline void EnvRoot::log_stat(const std::string& name, const T& value) {
    DLOG(INFO) << "stat: " << name << " = " << value;
    stat_current().add_stat(name, value);
}

inline Env::Env(Env&& other):
    m_root(std::move(other.m_root)),
    m_node(other.m_node),
    m_registry(std::move(other.m_registry)) {}

inline Env::Env(std::shared_ptr<EnvRoot> root,
                const AlgorithmValue& node,
                const Registry& registry):
    m_root(root),
    m_node(node),
    m_registry(std::make_unique<Registry>(registry)) {}

inline Env::~Env() = default;

inline const AlgorithmValue& Env::algo() const {
    return m_node;
}

inline std::shared_ptr<EnvRoot>& Env::root() {
    return m_root;
}

inline const Registry& Env::registry() const {
    return *m_registry;
}

inline void Env::error(const std::string& msg) {
    throw std::runtime_error(msg);
}

inline Env Env::env_for_option(const std::string& option) {
    CHECK(algo().arguments().count(option) > 0);
    auto& a = algo().arguments().at(option).as_algorithm();

    return Env(m_root, a, registry());
}

inline const OptionValue& Env::option(const std::string& option) const {
    return algo().arguments().at(option);
}

inline void Env::begin_stat_phase(const std::string& name) {
    m_root->begin_stat_phase(name); //delegate
}

inline void Env::end_stat_phase() {
    m_root->end_stat_phase(); //delegate
}

inline Stat& Env::finish_stats() {
    return m_root->finish_stats(); //delegate
}

inline void Env::restart_stats(const std::string& root_name) {
    m_root->restart_stats(root_name); //delegate
}

template<class T>
inline void Env::log_stat(const std::string& name, const T& value) {
    m_root->log_stat(name, value);
}

}

