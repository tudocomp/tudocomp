#pragma once

#include <tudocomp/util/algorithm_parser/ASTDef.hpp>
#include <tudocomp/util/algorithm_parser/AlgorithmAST.hpp>
#include <tudocomp/util/algorithm_parser/AlgorithmDecl.hpp>
#include <tudocomp/util/algorithm_parser/AlgorithmPattern.hpp>

#include <unordered_map>

#include <tudocomp/OptionValue.hpp>

namespace tdc {
    /// \cond INTERNAL
    namespace eval {
        using AlgorithmTypes = std::unordered_map<
            std::string, std::vector<decl::Algorithm>>;

        inline void check_string_not_string(ast::Value& v) {
            if (v.is_invokation()) {
                std::stringstream ss;
                ss << "option parser: ";
                ss << "trying to evaluate " << v << " as a string";
                throw std::runtime_error(ss.str());
            }
        }

        inline void check_static_override_fits_to_be_evaled_algo(
            ast::Value& fixed_static_args, ast::Value& v)
        {
            // TODO: Nice error
            // Check that the static "overrides" fit the
            // to-be-evaluated algorithm
            CHECK(fixed_static_args.is_invokation());
            CHECK_EQ(fixed_static_args.invokation_name(), v.invokation_name());
        }

        inline void check_arg_has_value(decl::Arg& arg, ast::Value& arg_value) {
            if (arg_value.is_empty()) {
                std::stringstream ss;
                ss << "option parser: ";
                ss << "argument '" << arg.name() << "' of type '"
                   << arg.type()
                   << "' has not been assigned a value";
                throw std::runtime_error(ss.str());
            }
            if (arg_value.is_invokation()) {
                if (std::any_of(arg_value.invokation_arguments().begin(),
                                arg_value.invokation_arguments().end(),
                                [](ast::Arg& arg) -> bool {
                                    return arg.has_type(); }))
                {
                    std::stringstream ss;
                    ss << "option parser: ";
                    ss << "argument '" << arg.name() << "' of type '"
                    << arg.type()
                    << "' has has types in its expression form: " << arg_value;
                    throw std::runtime_error(ss.str());
                }
            }
        }

        inline void check_algorithm_known(decl::Algorithm* found,
                            ast::Value& v,
                            std::vector<decl::Algorithm> candidates) {
            if (found == nullptr) {
                std::stringstream ss;

                ss << "option parser: ";
                ss << "algorithm '" << v.invokation_name() << "'";
                ss << " is not known to the evaluation engine. Currently known algorithms: [";
                for (auto& x : candidates) {
                    ss << x.name() << ", ";
                }
                ss << "]";

                throw std::runtime_error(ss.str());
            }
        }

        inline void check_argument_defined(const View& arg, const decl::Algorithm& algo) {
            for (auto& signature_arg : algo.arguments()) {
                if(arg == signature_arg.name()) return; // found
            }

            // not found
            std::stringstream ss;

            ss << "option parser: ";
            ss << "unknown option '" << arg << "' for ";
            ss << "algorithm '" << algo.name() << "'.";
            throw std::runtime_error(ss.str());
        }

        inline OptionValue eval(ast::Value&& v,
                                View type,
                                AlgorithmTypes& types,
                                bool pattern,
                                ast::Value&& fixed_static_args = ast::Value())
        {
            // Check for build-in types
            if (type == "string") {
                check_string_not_string(v);
                return OptionValue(std::move(v.string_value()));
            }

            bool has_fixed_static_args = !fixed_static_args.is_empty();

            if (has_fixed_static_args) {
                check_static_override_fits_to_be_evaled_algo(fixed_static_args, v);
                auto& a = fixed_static_args.invokation_arguments();
                std::reverse(a.begin(), a.end());
            }

            // Find the exact signature of the algorithm
            // TODO: Nice error
            CHECK(types.count(type) > 0);
            // TODO: Nice error
            CHECK(v.is_invokation());
            auto& candidates = types[type];
            decl::Algorithm* found = nullptr;
            for (auto& candidate : candidates) {
                if (candidate.name() == v.invokation_name()) {
                   found = &candidate;
                }
            }
            check_algorithm_known(found, v, candidates);

            // Signature found, evaluate by walking v and signature
            auto& v_signature = *found;

            // Prepare return value
            ds::InputRestrictionsAndFlags r_ds_flags = v_signature.textds_flags();
            std::string r_name = v_signature.name();
            std::vector<pattern::Arg> r_static_args;
            AlgorithmValue::ArgumentMap r_dynamic_args;

            // Step 1: Find argument name of each option in the invokation's
            // signature

            std::vector<View> v_argument_names;
            {
                // no positional allowed after the first keyword arg
                bool positional_ok = true;
                size_t i = 0;
                for (auto& unevaluated_arg : v.invokation_arguments()) {
                    // Sanity check, should be caught from outer recursive call
                    DCHECK(!unevaluated_arg.has_type());

                    // find argument name for each position in the
                    // unevaluated argument list
                    View argument_name("");
                    if (!unevaluated_arg.has_keyword()) {
                        // postional arg
                        // TODO: Nice error
                        CHECK(positional_ok);
                        // assume argument name from current position

                        // TODO: Nice error
                        argument_name = v_signature.arguments().at(i).name();
                        i++;
                    } else {
                        // keyword arg
                        positional_ok = false;
                        // assume argument name from keyword

                        argument_name = unevaluated_arg.keyword();
                    }

                    check_argument_defined(argument_name, v_signature);
                    v_argument_names.push_back(argument_name);
                }
            }

            // Step 2: Walk signature's arguments and produce evaluated
            //         data for each one

            for (auto& signature_arg : v_signature.arguments()) {
                if (pattern && !signature_arg.is_static()) {
                    // If we are evaluateing patterns, ignore dynamic arguments
                    continue;
                }

                int found = -1;
                for (size_t i = 0; i < v_argument_names.size(); i++) {
                    if (v_argument_names[i] == signature_arg.name()) {
                        found = i;
                    }
                }

                // logic here: if override by static,
                // try to use static
                // error on non-default given that mismatches
                // important!
                // merge, not use either-or!

                ast::Value arg_value;
                ast::Value arg_fixed_static_value;

                if (found != -1) {
                    // use
                    arg_value = v.invokation_arguments()[found].value();
                } else if (signature_arg.has_default()) {
                    arg_value = signature_arg.default_value();
                }

                // after value has been selected, override parts if static;

                if (has_fixed_static_args) {
                    if (signature_arg.is_static()) {
                        // TODO: Nice error
                        CHECK(fixed_static_args
                              .invokation_arguments()
                              .size() > 0);
                        {
                            auto& current_fixed_static = fixed_static_args
                                .invokation_arguments().back();

                            // TODO: Nice error
                            CHECK(current_fixed_static.keyword()
                                == signature_arg.name());

                            arg_fixed_static_value = current_fixed_static.value();

                            if (found == -1) {
                                arg_value = std::move(current_fixed_static.value());
                            }
                        }

                        fixed_static_args
                              .invokation_arguments()
                              .pop_back();
                    }
                }

                check_arg_has_value(signature_arg, arg_value);

                // Recursivly evaluate the argument
                OptionValue arg_evaluated
                    = eval(std::move(arg_value),
                           signature_arg.type(),
                           types,
                           pattern,
                           std::move(arg_fixed_static_value));

                if (signature_arg.is_static()
                    && arg_evaluated.as_algorithm().static_selection().name() != "")
                {
                    r_static_args.push_back(
                        pattern::Arg(
                            std::string(signature_arg.name()),
                            pattern::Algorithm(arg_evaluated.as_algorithm().static_selection())));
                }

                r_dynamic_args[signature_arg.name()]
                    = std::move(arg_evaluated);

            }

            // Step 2.5: Check that all fixed static args have been used

            if (has_fixed_static_args) {
                CHECK(fixed_static_args.invokation_arguments().size() == 0);
            }

            // Step 3: Return
            auto tmp = std::make_unique<pattern::Algorithm>(
                    std::string(r_name),
                    std::move(r_static_args));

            auto fr = OptionValue(AlgorithmValue(
                std::move(r_name),
                std::move(r_dynamic_args),
                std::move(tmp),
                r_ds_flags));

            return fr;
        }

        inline OptionValue cl_eval(ast::Value&& v,
                                   View type,
                                   AlgorithmTypes& types,
                                   ast::Value&& fixed_static_args = ast::Value()) {
            return eval(std::move(v),
                        type,
                        types,
                        false,
                        std::move(fixed_static_args));
        }
        inline pattern::Algorithm pattern_eval(ast::Value&& v,
                                               View type,
                                               AlgorithmTypes& types) {
            return std::move(eval(std::move(v),
                                  type,
                                  types,
                                  true).as_algorithm().static_selection());
        }
    }
    /// \endcond
}

