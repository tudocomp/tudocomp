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
    CompressorTest<LZCompressor>()
        .input("abcdebcdeabc")
        .threshold(2)
        .expected_rules(Rules { {5, 1, 4}, {9, 0, 3} })
        .run();
}

TEST(Roundtrip, LZCompressorCode0Coder) {
    test_roundtrip_batch<LZCompressor, Code0Coder>();
}
