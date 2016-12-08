#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <cstdlib>

#include "test_util.hpp"
#include <tudocomp_driver/Registry.hpp>
#include "tudocomp_driver_util.hpp"

using namespace tdc_algorithms;

// TODO
const std::vector<std::string> EXCLUDED_TESTS {
    "chain",
};
const std::vector<std::string> ADDITIONAL_TESTS {
    // "chain(chain(chain(chain(easyrle(\"1\"),bwt()),mtf()),easyrle()),encode(huff))",
    // "chain(lz78, lzw)",
    // "chain(lz78, chain(noop, lzw))",
};

std::vector<std::string> parse_scsv(const std::string& s) {
    std::vector<std::string> r;
    std::string remaining = s;

    while(!remaining.empty()) {
        auto next = remaining.find(";");
        r.push_back(remaining.substr(0, next));
        if (next == std::string::npos) {
            remaining = "";
        } else {
            remaining = remaining.substr(next + 1);
        }
    }

    return r;
}

TEST(TudocompDriver, roundtrip_matrix) {
    std::cout << "[ Generating list of test cases ]\n";

    std::vector<std::string> excluded_tests = EXCLUDED_TESTS;
    std::vector<std::string> additional_tests = ADDITIONAL_TESTS;
    std::vector<std::string> pattern_tests;
    bool has_pattern_tests = false;

    auto env_exclude_p = std::getenv("MATRIX_EXCLUDE");
    auto env_additional_p = std::getenv("MATRIX_ADDITIONAL");
    auto env_pattern_p = std::getenv("MATRIX_PATTERN");

    std::string env_exclude = (env_exclude_p != nullptr) ? env_exclude_p : "";
    std::string env_additional = (env_additional_p != nullptr) ? env_additional_p : "";
    std::string env_pattern = (env_pattern_p != nullptr) ? env_pattern_p : "";

    // Use cross product of all static arguments as base list to check
    std::vector<std::string> test_cases_pre_filter;

    for (const auto& x : REGISTRY.all_algorithms_with_static("compressor")) {
        test_cases_pre_filter.push_back(x.to_string(true));
    }

    for (auto x : parse_scsv(env_exclude)) {
        excluded_tests.push_back(x);
    }
    for (auto x : parse_scsv(env_pattern)) {
        pattern_tests.push_back(x);
        has_pattern_tests = true;
    }
    for (auto x : parse_scsv(env_additional)) {
        additional_tests.push_back(x);
    }

    auto test_cases_contains = [](const std::string test_case,
                                  const std::vector<std::string>& vs) {
        return std::any_of(vs.begin(),
                           vs.end(),
                           [&](const std::string& s) {
                            return test_case.find(s) != std::string::npos;
                           });
    };

    std::vector<std::string> test_cases;
    for (const auto& test_case: test_cases_pre_filter) {
        if (!test_cases_contains(test_case, excluded_tests)
            && (!has_pattern_tests || test_cases_contains(test_case, pattern_tests))
        ) {
            test_cases.push_back(test_case);
        }
    }

    for (const auto& x : additional_tests) {
        test_cases.push_back(x);
    }

    for (auto& e : test_cases) {
        std::cout << "  " << e << "\n";
    }
    std::cout << "[ Start roundtrip tests ]\n";

    std::vector<Error> errors;

    for (auto& algo : test_cases) {
        int counter = 0;
        bool abort = false;
        test_roundtrip_batch([&](std::string text) {
            if (abort) {
                return;
            }
            std::stringstream ss;
            ss << counter;
            std::string n = "_" + ss.str();
            counter++;

            auto e = roundtrip(algo, n, text, true, abort);
            if (e.has_error) {
                errors.push_back(e);
            }
        });
        if (abort) {
            break;
        }
    }

    for (auto& e : errors) {
        std::cout << "# [ERROR] ############################################################\n";
        std::cout << "  " << e.message << "\n";
        std::cout << "  in: " << e.test << "\n";
        if (e.text != e.roundtrip_text) {
            auto escaped_text = format_escape(e.text);
            auto escaped_roundtrip_text = format_escape(e.roundtrip_text);
            std::cout << "  expected:\n";
            std::cout << "  " << escaped_text << "\n";
            std::cout << "  actual:\n";
            std::cout << "  " << escaped_roundtrip_text << "\n";
            std::cout << "  diff:\n";
            std::cout << "  " << format_diff(e.text, e.roundtrip_text) << "\n";
        }
        std::cout << indent_lines(format_std_outputs({
            "compress command", e.compress_cmd,
            "compress stdout", e.compress_stdout,
            "decompress command", e.decompress_cmd,
            "decompress stdout", e.decompress_stdout,
        }), 2) << "\n";
        std::cout << "######################################################################\n";
    }

    ASSERT_TRUE(errors.empty());
}
