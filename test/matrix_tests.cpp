#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "test_util.hpp"
#include <tudocomp_driver/Registry.hpp>
#include "tudocomp_driver_util.hpp"

using namespace tdc_algorithms;

// TODO
const std::vector<std::string> excluded_tests {
    "chain",
};
const std::vector<std::string> additional_tests {
    "chain(chain(chain(chain(easyrle(\"1\"),bwt()),mtf()),easyrle()),encode(huff))",
    "chain(lz78, lzw)",
    "chain(lz78, chain(noop, lzw))",
};

TEST(TudocompDriver, roundtrip_matrix) {
    std::cout << "[ Generating list of test cases ]\n";

    // Use cross product of all static arguments as base list to check
    std::vector<std::string> test_cases_pre_filter;

    for (const auto& x : REGISTRY.all_algorithms_with_static("compressor")) {
        test_cases_pre_filter.push_back(x.to_string(true));
    }

    std::vector<std::string> test_cases;
    for (const auto& test_case: test_cases_pre_filter) {
        if (!std::any_of(excluded_tests.begin(),
                         excluded_tests.end(),
                        [&](const std::string& excluded) {
                            return test_case.find(excluded) == 0;
                        }))
        {
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
