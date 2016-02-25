#include <cstdint>
#include <iostream>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "test_util.h"

#include <tudocomp/lzw/lzw_compressor.h>
#include <tudocomp/lzw/bit_coder.h>
#include <tudocomp/lzw/dummy_coder.h>

using namespace lzw;

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
