#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "tudocomp.h"
#include "sa_compressor.h"

using namespace sa_compressor;

TEST(SACompressor, computeESA) {
    Input input = input_from_string("abcdebcdeabc");
    auto suffix = SACompressor::computeESA(input);
    decltype(suffix.sa) x { 12, 9, 0, 10, 5, 1, 11, 6, 2, 7, 3, 8, 4 };

    ASSERT_EQ(suffix.sa.size(), x.size());
    for (size_t i = 0; i < suffix.sa.size(); i++)
        ASSERT_EQ(suffix.sa[i], x[i]);
}
