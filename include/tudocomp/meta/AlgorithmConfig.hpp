#pragma once

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/Parser.hpp>

#include <unordered_map>

namespace tdc {
namespace meta {

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// \brief Represents a value configuration of a declared algorithm.
class AlgorithmConfig {
public:
    /// \brief Represents a value configuration for a declared parameter.
    class Param {
    private:
        const AlgorithmDecl::Param* m_decl;

        const ast::Node* m_config;
        std::vector<AlgorithmConfig> m_sub_configs; // valid if non-primitive

    public:
        /// \brief Main constructor.
        inline Param(
            const AlgorithmDecl::Param& decl,
            const ast::Node* config,
            const AlgorithmDict& dict)
            : m_decl(&decl), m_config(config) {

            // TODO
        }

        /// \brief Copy constructor.
        inline Param(const Param& other)
            : m_decl(other.m_decl),
              m_config(other.m_config),
              m_sub_configs(other.m_sub_configs) {
        }

        /// \brief Move constructor.
        inline Param(Param&& other)
            : m_decl(other.m_decl),
              m_config(other.m_config),
              m_sub_configs(std::move(other.m_sub_configs)) {
        }

        inline void add_sub_config(AlgorithmConfig&& sub) {
            m_sub_configs.emplace_back(std::move(sub));
        }

        inline std::string str() const {
            std::stringstream ss;
            ss << m_decl->name() << '=';

            if(m_decl->is_primitive()) {
                ss << m_config->str();
            } else {
                if(m_decl->is_list()) ss << '[';
                size_t i = 0;
                for(auto& sub : m_sub_configs) {
                    ss << sub.str();
                    if(++i < m_sub_configs.size()) ss << ", ";
                }
                if(m_decl->is_list()) ss << ']';
            }
            return ss.str();
        }
    };

private:
    const AlgorithmDecl* m_decl;
    std::vector<Param> m_params;

public:
    inline AlgorithmConfig(
        const AlgorithmDecl& decl,
        const ast::Node* config,
        const AlgorithmDict& dict)
        : m_decl(&decl) {

        // assure that config is an object
        auto obj = dynamic_cast<const ast::Object*>(config);
        if(!obj) throw ConfigError("invalid algorithm configuation");

        // sanity check that name matches
        if(m_decl->name() != obj->name()) {
            throw ConfigError("algorithm name mismatch");
        }

        // collect named and unnamed params in config
        std::vector<const ast::Param*> named_params;
        std::vector<const ast::Param*> unnamed_params;

        for(auto p : obj->params()) {
            if(p->has_name()) {
                named_params.emplace_back(p);
            } else {
                if(named_params.size() > 0) {
                    throw ConfigError("unnamed parameters need to come first");
                }

                unnamed_params.emplace_back(p);
            }
        }

        // create decl name mapping
        auto decl_params = m_decl->params();

        // sanity check
        if(decl_params.size() < named_params.size() + unnamed_params.size()) {
            throw ConfigError("too many parameters");
        }

        std::unordered_map<std::string, const AlgorithmDecl::Param*> unmapped;
        for(auto dp : decl_params) {
            unmapped.emplace(dp->name(), dp);
        }

        // attempt to map unnamed parameters first
        for(auto p : unnamed_params) {
            // TODO
        }

        // afterwards, attempt to map named parameters
        for(auto p : named_params) {
            // TODO
        }

        // finally, assign defaults to unmapped parameters
        for(auto& e : unmapped) {
            // TODO
        }
    }

    inline std::string str() const {
        std::stringstream ss;
        ss << m_decl->name() << '(';

        size_t i = 0;
        for(auto& param : m_params) {
            ss << param.str();
            if(++i < m_params.size()) ss << ", ";
        }

        ss << ')';
        return ss.str();
    }
};

}} //ns
