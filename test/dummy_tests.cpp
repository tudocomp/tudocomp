#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"

#include "tudocomp.h"

#include "dummy_encoder.h"
#include "dummy_compressor.h"

using namespace dummy;

// This is a gtest testcase definition.
// The code inside will run and use `ASSERT`, `ASSERT_EQ`
// and other gtest macros to test the code.
// For more details. see the gtest docs.
TEST(Dummy, compressor) {
    CompressorTest<DummyCompressor>()
        .input("abcdebcdeabc")
        .threshold(2)             // Threshold value is irrelevant here
        .expected_rules(Rules {}) // The dummy compressor does not produce any rules
        .run();
}

TEST(Dummy, encoder) {
    CoderTest<DummyCoder>()
        .input("abcdebcdeabc")
        .rules(Rules { {1, 5, 4}, {5, 10, 2} })
        .expected_output("abcdebcdeabc")
        .run();
}

TEST(Dummy, decoder) {
    DecoderTest<DummyCoder>()
        .input("abcdebcdeabc")
        .expected_output("abcdebcdeabc")
        .run();
}

TEST(Dummy, Roundrip) {
    // This is a template function included from test_util.h that
    // tests the given compresser-coder-decoder chain
    // with a number of ASCII and unicode strings
    test_roundtrip_batch<DummyCompressor, DummyCoder>();
}
