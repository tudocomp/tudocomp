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
            const AlgorithmDict& dict,
            bool list_item = false) {

            auto obj_value = ast::convert<ast::Object>(config,
                "type mismatch for " + param_str(list_item) +
                " '" + m_decl->name() + "'");

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
        /// \param decl the parameter declaration
        /// \param config the configured value
        /// \param dict the algorithm dictionary used for name resolution
        inline Param(
            const AlgorithmDecl::Param& decl,
            ast::NodePtr<> config,
            const AlgorithmDict& dict)
            : m_decl(&decl), m_config(config) {

            if(!m_config) throw ConfigError("null");

            if(m_decl->is_list()) {
                auto list_value = configure_list(m_config);

                //check list items
                if(m_decl->is_primitive()) {
                    for(auto& item : list_value->items()) {
                        configure_primitive(item, true);
                    }
                } else {
                    for(auto& item : list_value->items()) {
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

private:
    const AlgorithmDecl* m_decl;
    std::vector<Param> m_params;

public:
    /// \brief Main constructor.
    ///
    /// This will recursively create a configuration for all parameters and
    /// sub algorithms. A \ref ConfigError is thrown in case of a failure.
    ///
    /// \param decl the algorithm declaration
    /// \param config the value configuration
    /// \param dict the algorithm dictionary used for name resolution
    inline AlgorithmConfig(
        const AlgorithmDecl& decl,
        ast::NodePtr<> config,
        const AlgorithmDict& dict)
        : m_decl(&decl) {

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
                m_params.emplace_back(dp, p->value(), dict);

                mapped.emplace(dp.name());
                unmapped.erase(dp.name());
            }
        }

        // afterwards, attempt to map named parameters
        for(auto& p : named_params) {
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
        for(auto& e : unmapped) {
            auto dp = e.second;

            auto default_value = dp->default_value();
            if(!default_value) {
                throw ConfigError(
                    "parameter was given no value and has no default: '" +
                    dp->name() + "'");
            } else {
                m_params.emplace_back(*dp, default_value, dict);
            }
        }
    }

private:
    inline const Param& get_param(const std::string& name) const {
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
    inline T get(ast::NodePtr<> node) const {
        auto v = ast::convert<ast::Value>(node,
            "parameter has no primitive value type");

        return lexical_cast<T>(v->value());
    }

public:
    /// \brief Converts and returns the single value of the requested parameter.
    ///
    /// The parameter must exist and must have a primitive value, ie., it must
    /// neither be a sub configuration, nor a list of values.
    ///
    /// The value conversion is done using a \ref lexical_cast.
    ///
    /// \tparam T the value type to convert to
    /// \param param the name of the parameter
    /// \return the value of the parameter, converted to the desired type
    template<typename T>
    inline T get(const std::string& param) const {
        return get<T>(get_param(param).config());
    }

    /// \brief Gets the string value of the requested parameter.
    /// \param param the name of the parameter
    /// \return the string value of the parameter
    /// \see get
    inline std::string get_string(const std::string& param) const {
        return get<std::string>(param);
    }

    /// \brief Gets the boolean value of the requested parameter.
    ///
    /// The boolean value is evaluated using the \ref is_true function.
    ///
    /// \param param the name of the parameter
    /// \return the string value of the parameter
    /// \see get
    inline bool get_bool(const std::string& param) const {
        return is_true(get_string(param));
    }

    /// \brief Gets the integer value of the requested parameter.
    /// \param param the name of the parameter
    /// \return the integer value of the parameter
    /// \see get
    inline int get_int(const std::string& param) const {
        return get<int>(param);
    }

    /// \brief Gets the unsigned integer value of the requested parameter.
    /// \param param the name of the parameter
    /// \return the unsigned integer value of the parameter
    /// \see get
    inline unsigned int get_uint(const std::string& param) const {
        return get<unsigned int>(param);
    }

    /// \brief Gets the floating point value of the requested parameter.
    /// \param param the name of the parameter
    /// \return the floating point value of the parameter
    /// \see get
    inline float get_float(const std::string& param) const {
        return get<float>(param);
    }

    /// \brief Gets the double-precision floating point value of the
    ///        requested parameter.
    /// \param param the name of the parameter
    /// \return the double-precision floating point value of the parameter
    /// \see get
    inline double get_double(const std::string& param) const {
        return get<double>(param);
    }

    /// \brief Converts and returns the values of the requested list parameter.
    ///
    /// The parameter must exist and must be a list of primitive values. The
    /// value conversions are done as in the \ref get function.
    ///
    /// \tparam T the value type to convert single values to
    /// \param param the name of the parameter
    /// \return the values of the list parameter, converted to the desired type
    template<typename T>
    inline std::vector<T> get_vector(const std::string& param) const {
        auto list = ast::convert<ast::List>(get_param(param).config(),
            "parameter has no list value type");

        std::vector<T> vec;
        for(auto& item : list->items()) {
            vec.emplace_back(get<T>(item));
        }
        return vec;
    }

    /// \brief Gets the configuration of the requested sub algorithm.
    ///
    /// The parameter must exist and must be a single sub algorithm.
    ///
    /// \param param the name of the parameter
    /// \return the configuration of the corresponding sub algorithm
    inline const AlgorithmConfig& get_sub_config(
        const std::string& param) const {

        auto& sub = get_param(param).sub_configs();
        if(sub.size() == 0) {
            throw std::runtime_error("parameter has no sub configuations");
        } else if(sub.size() > 1) {
            throw std::runtime_error("parameter has multiple sub configuations");
        } else {
            return sub.front();
        }
    }

    /// \brief Gets the configurations of the requested sub algorithm list.
    ///
    /// The parameter must exist and must be a list of sub algorithms.
    ///
    /// \param param the name of the parameter
    /// \return the configurations of the corresponding sub algorithms
    inline const std::vector<AlgorithmConfig>& get_sub_configs(
        const std::string& param) const {

        return get_param(param).sub_configs();
    }

    /// \brief Gets the algorithm's declaration.
    /// \return the algorithm's declaration
    inline const AlgorithmDecl& decl() const {
        return *m_decl;
    }

    /// \brief Constructs the signature of the algorithm configuration,
    ///        with respect only to the configured sub algorithms.
    ///
    /// Signatures are used to map algorithm configurations to their
    /// corresponding C++ classes. These are usually instances of (cascaded)
    /// class templates, which the signature identifies uniquely.
    inline std::string signature() const {
        std::stringstream ss;
        size_t sig_params = 0;

        // add algorithm name to signature
        ss << m_decl->name();

        // iterate over declarated parameters
        for(auto& p : m_decl->params()) {
            if(!p.is_primitive()) {
                // add parameter to signature
                ss << (sig_params ? ", " : "(");
                ++sig_params;
                ss << p.name() << "=";

                if(p.is_list()) {
                    // list of sub algorithms
                    ss << "[";
                    auto& subs = get_sub_configs(p.name());
                    size_t i = 0;
                    for(auto& sub : subs) {
                        // recurse for each
                        ss << sub.signature();
                        if(++i < subs.size()) ss << ", ";
                    }
                    ss << "]";
                } else {
                    // single sub algorithm - recurse
                    ss << get_sub_config(p.name()).signature();
                }
            }
        }

        if(sig_params) ss << ")";

        return ss.str();
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

}} //ns
