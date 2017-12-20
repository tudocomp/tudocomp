#pragma once

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/Parser.hpp>
#include <tudocomp/meta/ast/TypeConversion.hpp>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <tudocomp/util.hpp>

namespace tdc {
namespace meta {

/// \brief Error type for config related errors.
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

        ast::NodePtr<> m_config;
        std::vector<AlgorithmConfig> m_sub_configs; // valid if non-primitive

        inline static std::string param_str(bool list_item) {
            return std::string(
                list_item ? "list item of parameter" : "parameter");
        }

        inline ast::NodePtr<ast::List> configure_list(ast::NodePtr<> config) {
            return ast::convert<ast::List>(config,
                "type mismatch for parameter '" + m_decl->name() + "'");
        }

        inline ast::NodePtr<ast::Value> configure_primitive(
            ast::NodePtr<> config,
            bool list_item = false) {

            return ast::convert<ast::Value>(config,
                "type mismatch for " + param_str(list_item) +
                " '" + m_decl->name() + "'");
        }

        inline ast::NodePtr<ast::Object> configure_object(
            ast::NodePtr<> config,
            const AlgorithmLib& lib,
            bool list_item = false) {

            auto obj_value = ast::convert<ast::Object>(config,
                "type mismatch for " + param_str(list_item) +
                " '" + m_decl->name() + "'");

            // find algorithm declaration
            auto it = lib.find(obj_value->name());
            if(it == lib.end()) {
                throw ConfigError("unknown algorithm type: '" +
                    obj_value->name() + "'");
            }

            auto algo_decl = it->second;

            // check algorithm type
            if(!algo_decl->type().subtype_of(m_decl->type())) {
                throw ConfigError("algorithm type mismatch for " +
                    param_str(list_item) + " '" + m_decl->name() + "': " +
                    "expected " + m_decl->type().name() +
                    ", got " + algo_decl->type().name());
            }

            // create sub config
            m_sub_configs.emplace_back(algo_decl, config, lib);

            return obj_value;
        }

    public:
        /// \brief Main constructor.
        /// \param decl the parameter declaration
        /// \param config the configured value
        /// \param lib the algorithm library used for name resolution
        inline Param(
            const AlgorithmDecl::Param& decl,
            ast::NodePtr<> config,
            const AlgorithmLib& lib)
            : m_decl(&decl), m_config(config) {

            if(!m_config) {
                throw ConfigError(
                    std::string("tried to configure parameter '") +
                    decl.name() + "' with a NULL value");
            }

            if(m_decl->is_list()) {
                auto list_value = configure_list(m_config);

                //check list items
                if(m_decl->is_primitive()) {
                    for(auto& item : list_value->items()) {
                        configure_primitive(item, true);
                    }
                } else {
                    for(auto& item : list_value->items()) {
                        configure_object(item, lib, true);
                    }
                }
            } else if(m_decl->is_primitive()) {
                configure_primitive(m_config);
            } else {
                configure_object(m_config, lib);
            }
        }

        /// \brief Copy constructor.
        /// \param other the object to copy
        inline Param(const Param& other)
            : m_decl(other.m_decl),
              m_config(other.m_config),
              m_sub_configs(other.m_sub_configs) {
        }

        /// \brief Move constructor.
        /// \param other the object to move
        inline Param(Param&& other)
            : m_decl(other.m_decl),
              m_config(other.m_config),
              m_sub_configs(std::move(other.m_sub_configs)) {
        }

        /// \brief Gets the parameter's declaration.
        /// \return the parameter's declaration
        inline const AlgorithmDecl::Param& decl() const {
            return *m_decl;
        }

        /// \brief Gets the parameter's configured value.
        /// \return the parameter's configured value
        inline ast::NodePtr<> config() const {
            return m_config;
        }

        /// \brief Gets the parameter's sub configurations.
        /// \return the parameter's sub configurations
        inline const std::vector<AlgorithmConfig>& sub_configs() const {
            return m_sub_configs;
        }

        /// \brief Returns a human-readable string representation of the
        ///        configuration.
        /// \return a human-readable string representation of the configuration
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

    /// \brief Accessor for parameter values.
    class ParamValue {
    private:
        friend class AlgorithmConfig;

        template<typename T>
        inline static T node_value_as(ast::NodePtr<> node) {
            auto v = ast::convert<ast::Value>(node,
                "parameter has no primitive value type");

            return lexical_cast<T>(v->value());
        }

        ast::NodePtr<> m_config;

        inline ParamValue(const Param& param) : m_config(param.config()) {}

    public:
        inline ast::NodePtr<> ast() const {
            return m_config;
        }

        /// \brief Converts and returns the single value of the parameter.
        ///
        /// The parameter must have a primitive value, ie., it must
        /// neither be a sub configuration, nor a list of values.
        ///
        /// The value conversion is done using a \ref lexical_cast.
        ///
        /// \tparam T the value type to convert to
        /// \return the value of the parameter, converted to the desired type
        template<typename T>
        inline T as() const {
            return node_value_as<T>(m_config);
        }

        /// \brief Gets the string value of the parameter.
        /// \return the string value of the parameter
        /// \see as
        inline std::string as_string() const {
            return as<std::string>();
        }

        /// \brief Gets the boolean value of the parameter.
        ///
        /// The boolean value is evaluated using the \ref is_true function.
        ///
        /// \return the boolean value of the parameter
        inline bool as_bool() const {
            return is_true(as_string());
        }

        /// \brief Gets the integer value of the parameter.
        /// \return the integer value of the parameter
        /// \see as
        inline int as_int() const {
            return as<int>();
        }

        [[deprecated("transitional alias")]]
        inline int as_integer() const {
            return as_int();
        }

        /// \brief Gets the unsigned integer value of the parameter.
        /// \return the unsigned integer value of the parameter
        /// \see as
        inline unsigned int as_uint() const {
            return as<unsigned int>();
        }

        /// \brief Gets the floating point value of the parameter.
        /// \return the floating point value of the parameter
        /// \see as
        inline float as_float() const {
            return as<float>();
        }

        /// \brief Gets the double-precision floating point value of the
        ///        parameter.
        /// \return the double-precision floating point value of the parameter
        /// \see as
        inline double as_double() const {
            return as<double>();
        }

        /// \brief Converts and returns the values of the list parameter.
        ///
        /// The parameter must be a list of primitive values. The value
        /// value conversions are done as in the \ref as function.
        ///
        /// \tparam T the value type to convert single values to
        /// \return the values of the list parameter
        template<typename T>
        inline std::vector<T> as_vector() const {
            auto list = ast::convert<ast::List>(m_config,
                "parameter has no list value type");

            std::vector<T> vec;
            for(auto& item : list->items()) {
                vec.emplace_back(node_value_as<T>(item));
            }
            return vec;
        }
    };

private:
    std::shared_ptr<const AlgorithmDecl> m_decl;
    std::vector<Param> m_params;

public:
    /// \brief Main constructor.
    ///
    /// This will recursively create a configuration for all parameters and
    /// sub algorithms. A \ref ConfigError is thrown in case of a failure.
    ///
    /// \param decl the algorithm declaration
    /// \param config the value configuration
    /// \param lib the algorithm library used for name resolution
    inline AlgorithmConfig(
        std::shared_ptr<const AlgorithmDecl> decl,
        ast::NodePtr<> config,
        const AlgorithmLib& lib)
        : m_decl(decl) {

        // assure that config is an object
        auto obj = ast::convert<ast::Object>(config,
            "invalid algorithm configuation");

        // sanity check that name matches
        if(m_decl->name() != obj->name()) {
            throw ConfigError("algorithm name mismatch");
        }

        // collect named and unnamed params in config
        std::vector<const ast::Param*> named_params;
        std::vector<const ast::Param*> unnamed_params;

        for(auto& p : obj->params()) {
            if(p.has_name()) {
                named_params.emplace_back(&p);
            } else {
                if(named_params.size() > 0) {
                    throw ConfigError(
                        "unnamed parameters need to be listed first");
                }

                unnamed_params.emplace_back(&p);
            }
        }

        // create decl name mapping
        auto& decl_params = m_decl->params();

        // sanity check
        if(decl_params.size() < named_params.size() + unnamed_params.size()) {
            throw ConfigError("too many parameters");
        }

        // store parameters that have not been mapped
        std::unordered_map<std::string, const AlgorithmDecl::Param*> unmapped;

        for(auto& dp : decl_params) {
            unmapped.emplace(dp.name(), &dp);
        }

        // store parameters that have been mapped
        std::unordered_set<std::string> mapped;

        // first, attempt to map unnamed parameters
        {
            size_t i = 0;
            for(auto& p : unnamed_params) {
                auto& dp = decl_params[i++];
                m_params.emplace_back(dp, p->value(), lib);

                mapped.emplace(dp.name());
                unmapped.erase(dp.name());
            }
        }

        // afterwards, attempt to map named parameters
        for(auto& p : named_params) {
            auto it = unmapped.find(p->name());
            if(it != unmapped.end()) {
                auto dp = it->second;
                m_params.emplace_back(*dp, p->value(), lib);

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
        for(auto& e : unmapped) {
            auto dp = e.second;

            auto default_value = dp->default_value();
            if(!default_value) {
                throw ConfigError(
                    "parameter was given no value and has no default: '" +
                    dp->name() + "'");
            } else {
                m_params.emplace_back(*dp, default_value, lib);
            }
        }
    }

    /// \brief Copy constructor.
    inline AlgorithmConfig(const AlgorithmConfig& other)
        : m_decl(other.m_decl), m_params(other.m_params) {
    }

    /// \brief Move constructor.
    inline AlgorithmConfig(AlgorithmConfig&& other)
        : m_decl(other.m_decl), m_params(std::move(other.m_params)) {
    }

private:
    inline const Param& get_param(const std::string& name) const {
        for(auto& p : m_params) {
            if(name == p.decl().name()) return p;
        }

        throw std::runtime_error("no such parameter: '" + name + "'");
    }

public:
    inline ParamValue param(const std::string& name) const {
        return ParamValue(get_param(name));
    }

    [[deprecated("transitional alias")]]
    inline ParamValue option(const std::string& name) const {
        return param(name);
    }

    /// \brief Gets the configuration of the requested sub algorithm.
    ///
    /// The parameter must exist and must be a single sub algorithm.
    ///
    /// \param param the name of the parameter
    /// \return the configuration of the corresponding sub algorithm
    inline const AlgorithmConfig& sub_config(const std::string& param) const {

        auto& sub = get_param(param).sub_configs();
        if(sub.size() == 0) {
            throw std::runtime_error("parameter has no sub configuations");
        } else if(sub.size() > 1) {
            throw std::runtime_error("parameter has multiple sub configuations");
        } else {
            return sub.front();
        }
    }

    [[deprecated("transitional solution")]]
    inline AlgorithmConfig env_for_option(
        const std::string& param) const {

        return AlgorithmConfig(sub_config(param));
    }

    /// \brief Gets the configurations of the requested sub algorithm list.
    ///
    /// The parameter must exist and must be a list of sub algorithms.
    ///
    /// \param param the name of the parameter
    /// \return the configurations of the corresponding sub algorithms
    inline const std::vector<AlgorithmConfig>& sub_configs(
        const std::string& param) const {

        return get_param(param).sub_configs();
    }

    /// \brief Gets the algorithm's declaration.
    /// \return the algorithm's declaration
    inline const AlgorithmDecl& decl() const {
        return *m_decl;
    }

    /// \brief Constructs the signature of the algorithm configuration,
    ///        with respect only to the configured bounded sub algorithms.
    ///
    /// Signatures are used to map algorithm configurations to their
    /// corresponding C++ classes. These are usually instances of (cascaded)
    /// class templates, which the signature identifies uniquely.
    inline ast::NodePtr<ast::Object> signature() const {
        auto sig = std::make_shared<ast::Object>(m_decl->name());

        // iterate over declarated parameters
        for(auto& p : m_decl->params()) {
            if(p.kind() == AlgorithmDecl::Param::Kind::bounded) {
                // add parameter to signature
                if(p.is_list()) {
                    // list of sub algorithms - recurse for each
                    auto list = std::make_shared<ast::List>();
                    for(auto& sub : sub_configs(p.name())) {
                        list->add_value(sub.signature());
                    }
                    sig->add_param(ast::Param(p.name(), list));
                } else {
                    // single sub algorithm - recurse
                    sig->add_param(ast::Param(
                        p.name(),
                        sub_config(p.name()).signature()));
                }
            }
        }
        return sig;
    }

    /// \brief Returns a human-readable string representation of the
    ///        configuration.
    /// \return a human-readable string representation of the configuration
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

} // namespace meta

using AlgorithmConfig = meta::AlgorithmConfig;

using Env = meta::AlgorithmConfig; //TODO: deprecate

} // namespace tdc
