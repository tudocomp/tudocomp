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
    const Input input = input_from_string("abcdebcdeabc");
    DummyCompressor compressor;
    Rules rules = compressor.compress(input, 2);

    // The dummy compressor does not produce any rules
    Rules should_be { };

    ASSERT_EQ(rules.size(), should_be.size());
    for (size_t i = 0; i < rules.size(); i++)
        ASSERT_EQ(rules[i], should_be[i]);
}

TEST(Dummy, encoder) {
    const Input input = input_from_string("abcdebcdeabc");

    Rules rules { {1, 5, 4}, {5, 10, 2} };

    // The dummy encoder just outputs the input data unchanged
    std::string should_be("abcdebcdeabc");

    DummyCoder coder;
    auto coded = ostream_to_string([&] (std::ostream& out) {
        coder.code(rules, input, 2, out);
    });
    ASSERT_EQ(should_be, coded);

}

TEST(Dummy, decoder) {
    const std::string should_be("abcdebcdeabc");

    std::istringstream inp("abcdebcdeabc");

    DummyCoder decoder;
    auto decoded = ostream_to_string([&] (std::ostream& out) {
        decoder.decode(inp, out);
    });
    ASSERT_EQ(should_be, decoded);
}

TEST(Dummy, Roundrip) {
    // This is a template function included from test_util.h that
    // tests the given compresser-coder-decoder chain
    // with a number of ASCII and unicode strings
    test_roundtrip_batch<DummyCompressor, DummyCoder>();
}
