#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <cstdlib>

#include <tudocomp_driver/Registry.hpp>

#include "test/util.hpp"
#include "test/driver_util.hpp"

using namespace tdc_algorithms;

const std::vector<std::string> EXCLUDED_TESTS {
    "chain",
};
const std::vector<std::string> ADDITIONAL_TESTS {
    // "chain(chain(chain(chain(easyrle(\"1\"),bwt()),mtf()),easyrle()),encode(huff))",
    // "chain(lz78, lzw)",
    // "chain(lz78, chain(noop, lzw))",
};

TEST(TudocompDriver, roundtrip_matrix) {
    std::cout << "[ Generating list of test cases ]\n";

    std::vector<std::string> test_cases;

    auto test_cases_contains = [](const std::string test_case,
                                  const std::vector<std::string>& vs) {
        return std::any_of(vs.begin(),
                           vs.end(),
                           [&](const std::string& s) {
                            return test_case.find(s) != std::string::npos;
                           });
    };

    // stage 1: automatically generated list of tests
    {
        for (const auto& x : REGISTRY.all_algorithms_with_static("compressor")) {
            test_cases.push_back(x.to_string());
        }
    }
    // stage 2: build-in exclude and additional
    {
        std::vector<std::string> test_cases_filtered;

        for (auto& x : test_cases) {
            if (!test_cases_contains(x, EXCLUDED_TESTS)) {
                test_cases_filtered.push_back(x);
            }
        }

        for (auto& x : ADDITIONAL_TESTS) {
            test_cases_filtered.push_back(x);
        }

        test_cases = test_cases_filtered;
    }

    // stage 3: environment exclude and additional
    {
        auto env_exclude_p = std::getenv("MATRIX_EXCLUDE");
        auto env_additional_p = std::getenv("MATRIX_ADDITIONAL");

        std::string env_exclude = (env_exclude_p != nullptr) ? env_exclude_p : "";
        std::string env_additional = (env_additional_p != nullptr) ? env_additional_p : "";

        std::vector<std::string> excluded_tests = driver_test::parse_scsv(env_exclude);
        std::vector<std::string> additional_tests = driver_test::parse_scsv(env_additional);

        std::vector<std::string> test_cases_filtered;

        for (auto& x : test_cases) {
            if (!test_cases_contains(x, excluded_tests)) {
                test_cases_filtered.push_back(x);
            }
        }

        for (auto& x : additional_tests) {
            test_cases_filtered.push_back(x);
        }

        test_cases = test_cases_filtered;
    }

    // stage 4: pattern filter
    {
        auto env_pattern_p = std::getenv("MATRIX_PATTERN");
        std::string env_pattern = (env_pattern_p != nullptr) ? env_pattern_p : "";

        std::vector<std::string> pattern_tests = driver_test::parse_scsv(env_pattern);

        if (!pattern_tests.empty()) {
            std::vector<std::string> test_cases_filtered;

            for(auto& x : test_cases) {
                if (test_cases_contains(x, pattern_tests)) {
                    test_cases_filtered.push_back(x);
                }
            }

            test_cases = test_cases_filtered;
        }
    }

    for (auto& e : test_cases) {
        std::cout << "  " << e << "\n";
    }
    std::cout << "[ Start roundtrip tests ]\n";

    std::vector<driver_test::Error> errors;

    for (auto& algo : test_cases) {
        int counter = 0;
        bool abort = false;
        test::roundtrip_batch([&](std::string text) {
            if (abort) {
                return;
            }
            std::stringstream ss;
            ss << std::setw(2) << std::setfill('0') << counter;
            std::string n = "_" + ss.str();
            counter++;

            auto e = driver_test::roundtrip(algo, n, text, true, abort);
            if (e.has_error) {
                errors.push_back(e);
            }
        });
    }

    for (auto& e : errors) {
        std::cout << "# [ERROR] ############################################################\n";
        std::cout << "  " << e.message << "\n";
        std::cout << "  in: " << e.test << "\n";
        if (e.text != e.roundtrip_text) {
            auto escaped_text = driver_test::format_escape(e.text);
            auto escaped_roundtrip_text = driver_test::format_escape(e.roundtrip_text);
            std::cout << "  expected:\n";
            std::cout << "  " << escaped_text << "\n";
            std::cout << "  actual:\n";
            std::cout << "  " << escaped_roundtrip_text << "\n";
            std::cout << "  diff:\n";
            std::cout << "  " << driver_test::format_diff(e.text, e.roundtrip_text) << "\n";
        }
        std::cout << indent_lines(driver_test::format_std_outputs({
            "compress command", e.compress_cmd,
            "compress stdout", e.compress_stdout,
            "decompress command", e.decompress_cmd,
            "decompress stdout", e.decompress_stdout,
        }), 2) << "\n";
        std::cout << "######################################################################\n";
    }

    ASSERT_TRUE(errors.empty());
}
