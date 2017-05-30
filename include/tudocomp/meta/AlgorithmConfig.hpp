#pragma once

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/Parser.hpp>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <tudocomp/util.hpp>

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

        inline static std::string param_str(bool list_item) {
            return std::string(
                list_item ? "list item of parameter" : "parameter");
        }

        inline const ast::List* configure_list(const ast::Node* config) {
            auto list_value = dynamic_cast<const ast::List*>(config);
            if(!list_value) {
                throw ConfigError("type mismatch for parameter '" +
                    m_decl->name() + "': expected list, got " +
                    config->debug_type());
            }
            return list_value;
        }

        inline const ast::Value* configure_primitive(
            const ast::Node* config,
            bool list_item = false) {

            auto value = dynamic_cast<const ast::Value*>(config);
            if(!value) {
                throw ConfigError("type mismatch for " + param_str(list_item) +
                    " '" + m_decl->name() + "': expected value, got " +
                    config->debug_type());
            }
            return value;
        }

        inline const ast::Object* configure_object(
            const ast::Node* config,
            const AlgorithmDict& dict,
            bool list_item = false) {

            auto obj_value = dynamic_cast<const ast::Object*>(config);
            if(!obj_value) {
                throw ConfigError("type mismatch for " + param_str(list_item) +
                    " '" + m_decl->name() + "': expected object, got " +
                    config->debug_type());
            }

            // find algorithm declaration
            auto it = dict.find(obj_value->name());
            if(it == dict.end()) {
                throw ConfigError("unknown algorithm type: '" +
                    obj_value->name() + "'");
            }

            auto& algo_decl = it->second;

            // check type (TODO: inheritance)
            if(algo_decl.type() != m_decl->type()) {
                throw ConfigError("algorithm type mismatch for " +
                    param_str(list_item) + " '" + m_decl->name() + "': " +
                    "expected " + m_decl->type() + ", got " + algo_decl.type());
            }

            // create sub config
            m_sub_configs.emplace_back(algo_decl, config, dict);

            return obj_value;
        }

    public:
        /// \brief Main constructor.
        inline Param(
            const AlgorithmDecl::Param& decl,
            const ast::Node* config,
            const AlgorithmDict& dict)
            : m_decl(&decl), m_config(config) {

            if(!m_config) throw ConfigError("null");

            if(m_decl->is_list()) {
                auto list_value = configure_list(m_config);

                //check list items
                if(m_decl->is_primitive()) {
                    for(auto item : list_value->items()) {
                        configure_primitive(item, true);
                    }
                } else {
                    for(auto item : list_value->items()) {
                        configure_object(item, dict, true);
                    }
                }
            } else if(m_decl->is_primitive()) {
                configure_primitive(m_config);
            } else {
                configure_object(m_config, dict);
            }
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

        inline const AlgorithmDecl::Param& decl() const { return *m_decl; }
        inline const ast::Node* config() const { return m_config; }

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
                    throw ConfigError(
                        "unnamed parameters need to be listed first");
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

        std::unordered_set<std::string> mapped;

        // first, attempt to map unnamed parameters
        {
            size_t i = 0;
            for(auto p : unnamed_params) {
                auto dp = decl_params[i++];
                m_params.emplace_back(*dp, p->value(), dict);

                mapped.emplace(dp->name());
                unmapped.erase(dp->name());
            }
        }

        // afterwards, attempt to map named parameters
        for(auto p : named_params) {
            auto it = unmapped.find(p->name());
            if(it != unmapped.end()) {
                auto dp = it->second;
                m_params.emplace_back(*dp, p->value(), dict);

                mapped.emplace(dp->name());
                unmapped.erase(dp->name());
            } else {
                if(mapped.find(p->name()) != mapped.end()) {
                    throw ConfigError("parameter '" + p->name() +
                        " already set");
                } else {
                    throw ConfigError("undefined parameter: '" + p->name() +
                        "'");
                }
            }
        }

        // finally, process unmapped parameters
        for(auto e : unmapped) {
            auto dp = e.second;

            //TODO: default value

            throw ConfigError(
                "parameter was given no value and has no default: '" +
                dp->name() + "'");
        }
    }

private:
    inline const Param& get_param(const std::string& name) {
        auto it = std::find_if(m_params.begin(), m_params.end(),
            [&](const Param& p) -> bool {
                return (name == p.decl().name());
            });

        if(it != m_params.end()) {
            return *it;
        } else {
            throw std::runtime_error("no such parameter: '" + name + "'");
        }
    }

    template<typename T>
    inline T get(const ast::Node* node) {
        auto v = dynamic_cast<const ast::Value*>(node);
        if(v) {
            return lexical_cast<T>(v->value());
        } else {
            throw std::runtime_error("parameter has no primitive value type");
        }
    }

public:
    template<typename T>
    inline T get(const std::string& param) {
        return get<T>(get_param(param).config());
    }

    inline std::string get_string(const std::string& param) {
        return get<std::string>(param);
    }

    inline bool get_bool(const std::string& param) {
        return is_true(get_string(param));
    }

    inline int get_int(const std::string& param) {
        return get<int>(param);
    }

    inline unsigned int get_uint(const std::string& param) {
        return get<unsigned int>(param);
    }

    inline int get_float(const std::string& param) {
        return get<float>(param);
    }

    template<typename T>
    inline std::vector<T> get_vector(const std::string& param) {
        auto list = dynamic_cast<const ast::List*>(get_param(param).config());
        if(list) {
            std::vector<T> vec;
            for(auto item : list->items()) {
                vec.emplace_back(get<T>(item));
            }
            return vec;
        } else {
            throw std::runtime_error("parameter has no list value type");
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
