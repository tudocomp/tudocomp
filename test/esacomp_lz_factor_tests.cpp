#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"

#include "tudocomp.h"
#include "lz_compressor.h"

#include "lz77rule_test_util.h"

using namespace esacomp;

TEST(LZCompressor, compress) {
    using namespace lz77rule_test;
    CompressorTest<LZCompressor>()
        .input("abcdebcdeabc")
        .threshold(2)
        .expected_rules(Rules {
            {5, 1, 4}, {9, 0, 3}
        })
        .run();
}
