#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "tudocomp.h"
#include "sa_compressor.h"
#include "test_util.h"

using namespace esacomp;

TEST(SACompressor, computeESA) {
    std::string input_string("abcdebcdeabc");
    std::vector<uint8_t> input_vec(input_string.begin(), input_string.end());
    Input input = Input::from_memory(input_vec);
    auto suffix = SACompressor::computeESA(input);
    decltype(suffix.sa) expected { 12, 9, 0, 10, 5, 1, 11, 6, 2, 7, 3, 8, 4 };

    assert_eq_integers(expected, suffix.sa);
}
