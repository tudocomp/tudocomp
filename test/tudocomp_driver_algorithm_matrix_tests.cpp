#include <stdio.h>
#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"
#include "tudocomp_driver/registry.h"
#include "tudocomp_driver/AlgorithmStringParser.hpp"

#include "tudocomp_driver_util.h"

TEST(TudocompDriver, all_compressors_defined) {
    using namespace tudocomp_driver;

    AlgorithmDb root;
    Registry registry {&root};
    register_algos(registry);
    auto s = registry.check_for_undefined_compressors();
    bool has_undefined_compressors = s.size() > 0;
    if (has_undefined_compressors) {
        std::stringstream ss;
        for (auto& s2 : s) {
            ss << "Undefined compressor: " << s2 << "\n";
        }
        EXPECT_FALSE(has_undefined_compressors) << ss.str();
    }
}

TEST(TudocompDriver, roundtrip_matrix) {
    std::cout << "[ Parsing Algorithm list from executable ]\n";
    auto list = list::tudocomp_list().root;
    std::cout << list.print("") << std::endl;
    std::cout << "[ Generating cross product of all algorithms ]\n";
    std::vector<std::string> algo_lines = algo_cross_product(list);
    for (auto& e : algo_lines) {
        std::cout << "  " << e << "\n";
    }
    std::cout << "[ Start roundtrip tests ]\n";

    for (auto& algo : algo_lines) {
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
