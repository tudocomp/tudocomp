#ifndef TUDOCOMP_REGISTRY_H
#define TUDOCOMP_REGISTRY_H

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <istream>
#include <map>
#include <sstream>
#include <streambuf>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <memory>

#include <glog/logging.h>

#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util.h>
#include <tudocomp/AlgorithmStringParser.hpp>

/// \brief Contains the executable driver application.
///
/// The driver application is a standalone executable that makes the
/// framework's compression and encoding algorithms available for use in a
/// command-line utility.
///
/// For algorithms to be made available in the driver application, they need
/// to be registered in the \ref Registry. Any registered algorithm will also
/// be listed in the utility's help message.
namespace tudocomp_algorithms {

using namespace tudocomp;

class Registry;

/// \cond INTERNAL
extern Registry REGISTRY;
/// \endcond

/// \brief Called when the driver application compiles its list of available
///        algorithms.
///
/// \param registry The application's registry.
void register_algorithms(Registry& registry);

/// \brief A registry for algorithms to be made available in the driver
///        application.
///
/// For algorithms to be made available in the driver application, they need
/// to be registered in the application's main registry. This is done in
/// the \ref register_algorithms step. Any registered algorithm will also
/// be listed in the utility's help message.
class Registry {
    eval::AlgorithmTypes m_algorithms;
    std::map<pattern::Algorithm, CompressorConstructor> m_compressors;

    /// \cond INTERNAL
    friend class AlgorithmTypeBuilder;
    friend class GlobalRegistry;
    /// \endcond

    inline Registry() {}

public:
    /// \brief Registers a \ref tudocomp::Compressor.
    ///
    /// Note that the compressor type \c T needs to implement a static function
    /// called \c meta() that returns a \ref tudocomp::Meta information object.
    /// This meta information is used to automatically generate the
    /// documentation for the driver application's help message.
    ///
    /// \tparam T The compressor to register.
    template<class T>
    void register_compressor() {
        auto meta = T::meta();

        ast::Value s = std::move(meta).build_static_args_ast_value();

        gather_types(m_algorithms, std::move(meta));

        auto static_s
            = eval::pattern_eval(std::move(s), "compressor", m_algorithms);

        CHECK(m_compressors.count(static_s) == 0); // Don't register twice...
        m_compressors[std::move(static_s)] = [](Env&& env) {
            return std::make_unique<T>(std::move(env));
        };
    }

    /// \cond INTERNAL

    // Create the list of all possible static-argument-type combinations
    std::vector<pattern::Algorithm> all_algorithms_with_static(View type) {
        std::vector<pattern::Algorithm> r;

        using AlgorithmArgs = std::vector<pattern::Arg>;

        for (auto& c : m_algorithms[type]) {
            std::vector<std::vector<AlgorithmArgs>> args_variations;

            for (auto& arg : c.arguments()) {
                std::string& arg_name = arg.name();
                CHECK(arg_name.size() > 0);

                auto arg_type = arg.type();
                bool is_static = arg.is_static();
                if (is_static) {
                    std::vector<AlgorithmArgs> arg_variations;
                    for(auto arg : all_algorithms_with_static(arg_type)) {
                        arg_variations.push_back(AlgorithmArgs {
                            pattern::Arg {
                                std::string(arg_name),
                                std::move(arg)
                            }
                        });
                    }
                    args_variations.push_back(arg_variations);
                }
            }

            std::string x_name;
            std::vector<pattern::Arg> x_args;

            x_name = c.name();
            std::vector<AlgorithmArgs> r_;
            if (args_variations.size() == 0) {
                pattern::Algorithm x {
                    std::move(x_name),
                    std::move(x_args)
                };

                r.push_back(x);
            } else {
                r_ = cross<AlgorithmArgs>(
                    std::move(args_variations), [](AlgorithmArgs s,
                                                   AlgorithmArgs& t) {
                    for (auto& e : t) {
                        s.push_back(e);
                    }
                    return s;
                });
            }

            for (auto& elem : r_) {
                r.push_back(pattern::Algorithm {
                    std::string(c.name()),
                    std::move(elem)
                });
            }
        }

        return r;
    }

    std::vector<pattern::Algorithm> check_for_undefined_compressors() {
        std::vector<pattern::Algorithm> r;
        for (auto& s : all_algorithms_with_static("compressor")) {
            if (m_compressors.count(s) == 0) {
                r.push_back(s);
            }
        }
        return r;
    }

    inline std::unique_ptr<Compressor> select_algorithm_or_exit(string_ref text) {
        ast::Parser p { text };
        auto parsed_algo = p.parse_value();
        auto evald_algo = eval::cl_eval(std::move(parsed_algo),
                                        "compressor",
                                        m_algorithms);

        auto& static_only_evald_algo = evald_algo.static_selection;
        auto& options = evald_algo.options;

        if (m_compressors.count(static_only_evald_algo) > 0) {
            auto env = std::make_shared<EnvRoot>(
                std::move(options.as_algorithm()));

            auto& constructor = m_compressors[static_only_evald_algo];
            return constructor(Env(env, env->algo_value()));
        } else {
            throw std::runtime_error("No implementation found for algorithm "
            + static_only_evald_algo.to_string()
            );
        }
    }

    inline static Registry with_all_from(std::function<void(Registry&)> f) {
        Registry r;
        f(r);
        return std::move(r);
    }


    inline std::string generate_doc_string() {
        auto print = [](std::vector<decl::Algorithm>& x, size_t iden) {
            std::vector<std::string> cells;

            for (auto& y : x) {
                auto spec = y.to_string(true);

                std::stringstream where;
                bool first = true;
                for (auto& z : y.arguments()) {
                    if (first) {
                        where << "\n  where ";
                    } else {
                        where << "\n        ";
                    }
                    first = false;
                    where << "`" << z.name() << "` is one of [" << z.type() << "],";
                }
                auto s = spec + where.str();
                if (y.arguments().size() > 0) {
                    s = s.substr(0, s.size() - 1);
                }
                cells.push_back(s);
                cells.push_back(y.doc());
            }

            return indent_lines(make_table(cells, 2), iden);
        };

        std::stringstream ss;

        ss << "  [Compression algorithms]\n";
        ss << print(m_algorithms["compressor"], 2) << "\n\n";

        ss << "  [Argument types]\n";
        for (auto& x : m_algorithms) {
            if (x.first == "compressor") {
                continue;
            }
            ss << "    [" << x.first << "]\n";
            ss << print(x.second, 4) << "\n\n";
        }

        return ss.str();
    }

    /// \endcond
};

}

#endif
