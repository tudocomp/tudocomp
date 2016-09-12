#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "test_util.h"
#include <tudocomp_driver/Registry.hpp>
#include "tudocomp_driver_util.h"

using namespace tudocomp_driver;

// TODO
const std::vector<std::string> additional_tests = {};

TEST(TudocompDriver, roundtrip_matrix) {
    std::cout << "[ Generating list of test cases ]\n";

    // Use cross product of all static arguments as base list to check
    std::vector<std::string> test_cases;

    for (const auto& x : REGISTRY.all_algorithms_with_static("compressor")) {
        test_cases.push_back(x.to_string(true));
    }
    for (const auto& x : additional_tests) {
        test_cases.push_back(x);
    }

    for (auto& e : test_cases) {
        std::cout << "  " << e << "\n";
    }
    std::cout << "[ Start roundtrip tests ]\n";

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

            roundtrip(algo, n, text, true, abort);
        });
        if (abort) {
            break;
        }
    }
}
