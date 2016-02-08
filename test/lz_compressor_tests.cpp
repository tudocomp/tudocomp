#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"

#include "tudocomp.h"
#include "lz_compressor.h"
#include "lz78.h"
#include "lz77.h"
#include "lzw.h"
#include "code0.h"

#include "lz78rule.h"

#include "lz77rule_test_util.h"
#include "lz78rule_test_util.h"

using namespace lz_compressor;
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

TEST(LZ78, compress) {
    // TODO:
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

TEST(LZ77Classic, test) {
    using namespace lz77rule_test;
    CompressorTest<LZ77ClassicCompressor>()
        .input("aacaacabcabaaac")
        .threshold(0)
        .expected_rules(Rules {
            {1, 0, 1}, {3, 0, 4}, {8, 5, 3}, {12, 0, 3}
        })
        .run();
}

TEST(LZW, DebugCode_compress) {
    Env env;
    LZWDebugCode coder(env);
    Input inp = Input::from_memory("LZWLZ78LZ77LZCLZMWLZAP");

    LzwRule compressor(env, &coder);

    std::stringstream ss;
    Output out = Output::from_stream(ss);
    compressor.compress(inp, out);

    ASSERT_EQ("'L','Z','W',256,'7','8',259,'7',256,'C',256,'M',258,'Z','A','P',",
              ss.str());
}

TEST(LZW, DebugCode_decompress) {
    Env env;
    LZWDebugCode coder(env);
    Input inp = Input::from_memory("'L','Z','W',256,'7','8',259,'7',256,'C',256,'M',258,'Z','A','P',");

    LzwRule compressor(env, &coder);

    std::stringstream ss;
    Output out = Output::from_stream(ss);
    compressor.decompress(inp, out);

    ASSERT_EQ("LZWLZ78LZ77LZCLZMWLZAP",
              ss.str());
}

TEST(LZW, BitCode_compress) {
    Env env;
    LZWBitCode coder(env);
    Input inp = Input::from_memory("abcdebcdeabc");

    LzwRule compressor(env, &coder);

    std::vector<uint8_t> bits;
    Output out = Output::from_memory(bits);
    compressor.compress(inp, out);

    ASSERT_EQ(pack_integers({
        9, 64,
        8, 6,
        'a', 9,
        'b', 9,
        'c', 9,
        'd', 9,
        'e', 9,
        257, 9,
        259, 9,
        256, 9,
        'c', 9,
    }), bits);
}

TEST(LZW, BitCode_decompress) {
    Env env;
    LZWBitCode coder(env);
    std::vector<uint8_t> bits = pack_integers({
        9, 64,
        8, 6,
        'a', 9,
        'b', 9,
        'c', 9,
        'd', 9,
        'e', 9,
        257, 9,
        259, 9,
        256, 9,
        'c', 9,
    });

    Input inp = Input::from_memory(bits);

    LzwRule compressor(env, &coder);

    std::stringstream ss;
    Output out = Output::from_stream(ss);
    compressor.decompress(inp, out);

    ASSERT_EQ("abcdebcdeabc",
              ss.str());
}
