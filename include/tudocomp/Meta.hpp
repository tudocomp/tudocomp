#pragma once

#include <functional>
#include <memory>

#include <tudocomp/AlgorithmStringParser.hpp>
#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

class Meta;

/// \cond INTERNAL
inline void gather_types(eval::AlgorithmTypes& target, std::vector<Meta>&& metas);
/// \endcond

/// \brief Provides meta information about an Algorithm.
///
/// Any Algorithm must implement a static function \c meta() that returns an
/// instance of this class, providing meta information about it.
///
/// The information is used for the automatic instantiation methods (e.g.
/// \ref create_algo) and for automatically generated documentation in the
/// driver application.
///
/// It consists of an algorithm \e type, the \e name, additional documentation,
/// accepted environment \e option and allows the definition of a hierarchy of
/// algorithms. For instance, an algorithm may accept other algorithms of a
/// type \c "strategy" as options, allowing for dynamic switching of strategies
/// based on the application. Please refer to the tutorial documents for more
/// information on this topic.
class Meta {
    std::string m_type;
    std::string m_name;
    std::string m_docs;
    ds::InputRestrictionsAndFlags m_ds_flags;

    std::vector<decl::Arg> m_options;

    ast::Value m_static_args;

    std::vector<Meta> m_sub_metas;

    inline void check_arg(const std::string& argument_name) {
        for (auto& e : m_options) {
            if (e.name() == argument_name) {
                throw std::runtime_error("argument already defined");
            }
        }
    }

    friend inline void gather_types(eval::AlgorithmTypes& target,
                                    Meta&& meta);
public:

    /// \brief Constructs an algorithm meta object with the given information.
    ///
    /// \param type The algorithm type. This algorithm can be passed as an
    ///             option to other algorithms that accept the same type.
    /// \param name The name of the algorithm, used for the auto-generated
    ///             documentation.
    /// \param doc Further documentation text, used for the auto-generated
    ///             documentation.
    inline Meta(const std::string& type,
                const std::string& name,
                const std::string& doc = ""):
        m_type(type),
        m_name(name),
        m_docs(doc),
        m_ds_flags(),
        m_static_args(ast::Value(std::string(name), {}))
    {
    }

    /// \brief Helper for declaring accepted options in algorithm meta
    ///        information.
    class OptionBuilder {
        Meta& m_meta;
        std::string m_argument_name;
    public:

        /// \brief Constructor. Do not use, construct via \ref Meta::option instead.
        OptionBuilder(Meta& meta, const std::string& argument_name):
            m_meta(meta), m_argument_name(argument_name) {}

        /// \brief Declares that this option accepts values of the specified
        ///        Algorithm type \c T.
        ///
        /// \tparam T The Algorithm type.
        template<class T>
        inline void templated(const std::string& accepted_type) {
            m_meta.check_arg(m_argument_name);
            Meta sub_meta = T::meta();
            m_meta.m_sub_metas.push_back(sub_meta);

            auto type = std::move(sub_meta.m_type);

            m_meta.m_static_args.invokation_arguments().push_back(ast::Arg(
                std::string(m_argument_name),
                std::move(sub_meta).build_static_args_ast_value()
            ));

            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                true,
                std::move(type)
            ));
        }

        /// \brief Declares that this option accepts values of the specified
        ///        Algorithm type \c T. Its meta information is gathered from
        ///        type \c D.
        ///
        /// Note that type \c D is required to implement a static function
        /// called \c meta() that returns a \ref Meta object containing
        /// information about type \c T.
        ///
        /// \tparam T The Algorithm type.
        /// \tparam D The meta information provider type. \e Must implement a
        ///           static function \ref Meta \c meta().
        template<class T, class D>
        inline void templated(const std::string& accepted_type) {
            templated<T>(accepted_type);

            std::string t_type = m_meta.m_options.back().type();
            m_meta.m_options.pop_back();

            Meta sub_meta = D::meta();
            std::string d_type = sub_meta.m_type;
            m_meta.m_sub_metas.push_back(sub_meta);

            DCHECK_EQ(t_type, d_type);

            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                true,
                std::move(sub_meta.m_type),
                std::move(sub_meta).build_ast_value_for_default()
            ));
        }

        /// \brief Declares that this option accepts values of a simple type
        ///        that can be parsed from a string (e.g. integers).
        ///
        /// The option will have no default value.
        inline void dynamic() {
            m_meta.check_arg(m_argument_name);
            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                false,
                "string"
            ));
        }

        /// \brief Declares that this option accepts values of a simple type
        ///        that can be parsed from a string (e.g. integers).
        /// \param default_value the default value for the option.
        inline void dynamic(const std::string& default_value) {
            m_meta.check_arg(m_argument_name);
            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                false,
                "string",
                ast::Value(std::string(default_value))
            ));
        }

        /// \cond INTERNAL
        //yes, this is necessary
        inline void dynamic(const char* default_value) {
            dynamic(std::string(default_value));
        }
        /// \endcond

        /// \brief Declares that this option accepts values of a simple type
        ///        that can be parsed from a string (e.g. integers).
        /// \param default_value the default value for the option.
        inline void dynamic(bool default_value) {
            dynamic(default_value ? "true" : "false");
        }

        /// \brief Declares that this option accepts values of a simple type
        ///        that can be parsed from a string (e.g. integers).
        /// \param default_value the default value for the option.
        inline void dynamic(int default_value) { dynamic(to_str(default_value)); }

        /// \brief Declares that this option accepts values of a simple type
        ///        that can be parsed from a string (e.g. integers).
        /// \param default_value the default value for the option.
        inline void dynamic(float default_value) { dynamic(to_str(default_value)); }

        /// \brief Declares that this option accepts values of a simple type
        ///        that can be parsed from a string (e.g. integers).
        /// \param default_value the default value for the option.
        inline void dynamic(double default_value) { dynamic(to_str(default_value)); }

        /// \brief Declares that this option accepts values of a arbitrary
        ///        Compressor type, dispatched at runtime.
        inline void dynamic_compressor() {
            m_meta.check_arg(m_argument_name);
            m_meta.m_options.push_back(decl::Arg(
                std::string(m_argument_name),
                false,
                "compressor"
            ));
        }
    };

    /// \brief Declares an accepted option for this algorithm.
    ///
    /// \param name The option's name.
    inline OptionBuilder option(const std::string& name) {
        return OptionBuilder(*this, name);
    }

    /// \cond INTERNAL

    inline decl::Algorithm build_def() && {
        return decl::Algorithm(
            std::move(m_name),
            std::move(m_options),
            std::move(m_docs),
            m_ds_flags
        );
    }

    inline ast::Value build_ast_value_for_default() && {
        std::vector<ast::Arg> options;
        for (auto& arg : m_options) {
            options.push_back(ast::Arg(
                arg.name(),
                std::move(arg.default_value())
            ));
        }

        return ast::Value(std::move(m_name), std::move(options));
    }

    inline ast::Value build_static_args_ast_value() && {
        return std::move(m_static_args);
    }

    /// \endcond

    /// \brief Returns the algorithm's name.
    /// \return The algorithm's name.
    inline const std::string& name() const {
        return m_name;
    }

    /// \brief Returns the algorithm's type.
    /// \return The algorithm's type.
    inline const std::string& type() const {
        return m_type;
    }

    /// \brief Places byte restrictions on the Input.
    inline void input_restrictions(const io::InputRestrictions& restr) {
        m_ds_flags |= restr;
    }

    /// \deprecated
    /// \brief Indicates that this Algorithm requires a null terminator symbol in Input.
    ///
    /// All occurrences of null in the input will be escaped,
    /// and an extra null will be appended to it.
    inline void needs_sentinel_terminator() {
        m_ds_flags |= io::InputRestrictions({ 0 }, true);
    }

    /// \brief Indicates that this Algorithm uses the TextDS class, and how it does.
    template<typename text_t>
    inline void uses_textds(ds::dsflags_t flags) {
        io::InputRestrictions existing = m_ds_flags;
        ds::InputRestrictionsAndFlags r(text_t::common_restrictions(flags) | existing,
                                        flags);
        m_ds_flags = r;
    }

    /// \cond INTERNAL
    inline ds::InputRestrictionsAndFlags textds_flags() {
        return m_ds_flags;
    }
    /// \endcond
};

/// \cond INTERNAL
inline void gather_types(eval::AlgorithmTypes& target,
                         Meta&& meta)
{
    // get vector for the current type
    auto& target_type_algos = target[meta.type()];

    // Create decl::Algorithm value here

    auto decl_value = std::move(meta).build_def();

    bool found = false;
    for (auto& already_seen_algo : target_type_algos) {
        if (already_seen_algo.name() == decl_value.name()) {
            if (already_seen_algo != decl_value) {
                std::stringstream ss;
                ss << "Attempting to register the same algorithm twice";
                ss << " with differing details:";
                ss << " new: " << decl_value;
                ss << " existing: " << already_seen_algo;
                throw std::runtime_error(ss.str());
            }

            found = true;
            break;
        }
    }

    if (!found) {
        target_type_algos.push_back(std::move(decl_value));
    }

    gather_types(target, std::move(meta.m_sub_metas));
}

inline void gather_types(eval::AlgorithmTypes& target,
                         std::vector<Meta>&& metas)
{
    for (auto& meta : metas) {
        gather_types(target, std::move(meta));
    }
}
/// \endcond

}

