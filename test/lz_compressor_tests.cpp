#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"

#include "tudocomp.h"
#include "lz_compressor.h"
#include "code0.h"

using namespace lz_compressor;
using namespace esacomp;

TEST(LZCompressor, compress) {
    const Input input = input_from_string("abcdebcdeabc");
    LZCompressor compressor;
    Rules rules = compressor.compress(input, 2);
    Rules x { {5, 1, 4}, {9, 0, 3} };

    ASSERT_EQ(rules.size(), x.size());
    for (size_t i = 0; i < rules.size(); i++)
        ASSERT_EQ(rules[i], x[i]);
}

TEST(Roundtrip, LZCompressorCode0Coder) {
    test_roundtrip_batch<LZCompressor, Code0Coder>();
}
