#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"

#include "tudocomp.h"
#include "lz77rule.h"
#include "lz_compressor.h"
#include "lz78.h"
#include "code0.h"

#include "lz78rule.h"

using namespace lz77rule;
using namespace lz_compressor;
using namespace esacomp;

TEST(LZCompressor, compress) {
    CompressorTest<LZCompressor>()
        .input("abcdebcdeabc")
        .threshold(2)
        .expected_rules(Rules {
            {5, 1, 4}, {9, 0, 3}
        })
        .run();
}

TEST(Roundtrip, LZCompressorCode0Coder) {
    test_roundtrip_batch(lz77roundtrip<LZCompressor, Code0Coder>);
}

TEST(LZ78, compress) {
    /*CompressorTest<LZ78Compressor>()
        .input("abaaabab")
        .threshold(0)
        .expected_rules(Rules {
            // (0,0)a(0,0)b(0,1)a(0,1)b(4,2)
            {0, 0, 0}, {1, 0, 0}, {2, 0, 1}, {4, 0, 1}, {6, 4, 2}
        })
        .run();
    CompressorTest<LZ78Compressor>()
        .input("abcdebcdeabc")
        .threshold(1)
        .expected_rules(Rules {
            {5, 1, 1}, {7, 3, 1}, {9, 0, 1}, {11, 2, 1}
        })
        .run();
    CompressorTest<LZ78Compressor>()
        .input("ananas")
        .threshold(0)
        .expected_rules(Rules {
            // (0,a)(0,n)(1,n)(1,s)
            {0, 0, 0}, {1, 0, 0}, {2, 0, 1}, {4, 0, 1}
        })
        .run();*/
}

template<class Comp, class Cod>
void lz78roundtrip(const std::string input_string) {
    Env env;

    Comp compressor { env };

    const Input input = input_from_string(input_string);

    DLOG(INFO) << "LZ78 ROUNDTRIP TEXT: " << input_string;

    Entries entries = compressor.compress(input);

    DLOG(INFO) << "LZ78 ROUNDTRIP PRE ENTRIES";

    for (auto e : entries) {
        DLOG(INFO) << "LZ78 ROUNDTRIP ENTRY: " << e;
    }

    Cod coder { env };

    // Encode input with rules
    std::string coded_string = ostream_to_string([&] (std::ostream& out) {
        coder.code(entries, input, out);
    });

    DLOG(INFO) << "ROUNDTRIP CODED: " << vec_to_debug_string(coded_string);

    //Decode again
    std::istringstream coded_stream(coded_string);
    std::string decoded_string = ostream_to_string([&] (std::ostream& out) {
        coder.decode(coded_stream, out);
    });

    DLOG(INFO) << "ROUNDTRIP DECODED: " << decoded_string;

    assert_eq_strings(input_string, decoded_string);
}

TEST(Roundtrip, LZ78CompressorDebugCode) {
    test_roundtrip_batch(lz78roundtrip<LZ78Compressor, LZ78DebugCode>);
}

TEST(Roundtrip, LZ78CompressorBitCode) {
    test_roundtrip_batch(lz78roundtrip<LZ78Compressor, LZ78BitCode>);
}
