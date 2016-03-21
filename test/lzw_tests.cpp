#include <cstdint>
#include <iostream>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "test_util.h"
#include "tudocomp_test_util.h"

#include <tudocomp/lzw/lzw_compressor.h>
#include <tudocomp/lzw/bit_coder.h>
#include <tudocomp/lzw/dummy_coder.h>

#include <tudocomp/lzw/LzwCompressor.hpp>
#include <tudocomp/lzw/LzwBitCoder.hpp>
#include <tudocomp/lzw/LzwDebugCoder.hpp>

using namespace lzw;
using lz78_dictionary::DMS_MAX;

TEST(Lzw, compress) {
    using Coder = tudocomp::lzw::LzwDebugCoder;
    using Compressor = tudocomp::lzw::LzwCompressor<Coder>;

    {
        auto encoded = test::compress<Compressor>("LZWLZ78LZ77LZCLZMWLZAP");
        ASSERT_EQ(encoded.str,
            "'L','Z','W',256,'7','8',259,'7',256,'C',256,'M',258,'Z','A','P',");
        encoded.assert_decompress();
    }
    {
        auto encoded = test::compress<Compressor>("abcdebcdeabcd");
        ASSERT_EQ(encoded.str, "'a','b','c','d','e',257,259,256,258,");
        encoded.assert_decompress();
    }
    {
        auto encoded = test::compress<Compressor>("abcdebcdeabc");
        ASSERT_EQ(encoded.str, "'a','b','c','d','e',257,259,256,'c',");
        encoded.assert_decompress();
    }
    {
        auto encoded = test::compress<Compressor>("xyxaybxa!xa!?");
        ASSERT_EQ(encoded.str, "'x','y','x','a','y','b',258,'!',262,'?',");
        encoded.assert_decompress();
    }
}

TEST(OldLzw, DebugCode_compress) {
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

TEST(OldLzw, DebugCode_compress2) {
    Env env;
    LZWDebugCode coder(env);
    Input inp = Input::from_memory("abcdebcdeabcd");

    LzwRule compressor(env, &coder);

    std::stringstream ss;
    Output out = Output::from_stream(ss);
    compressor.compress(inp, out);

    ASSERT_EQ("'a','b','c','d','e',257,259,256,258,", ss.str());
}

TEST(OldLzw, DebugCode_decompress) {
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

TEST(OldLzw, BitCode_compress) {
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

TEST(OldLzw, BitCode_decompress) {
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

TEST(Lzw, LzwDebugCoder) {
    using Coder = tudocomp::lzw::LzwDebugCoder;

    Env env;
    std::vector<uint8_t> encoded_buffer;
    Output encoded_out = Output::from_memory(encoded_buffer);

    {
        Coder coder(env, encoded_out);

        coder.encode_fact('x');
        coder.encode_fact('y');
        coder.encode_fact('x');
        coder.encode_fact('a');
        coder.encode_fact('y');
        coder.encode_fact('b');
        coder.encode_fact(258);
        coder.encode_fact('!');
        coder.encode_fact(262);
        coder.encode_fact('?');
    }

    auto encoded = std::string(encoded_buffer.begin(), encoded_buffer.end());
    ASSERT_EQ(encoded, "'x','y','x','a','y','b',258,'!',262,'?',");

    auto input = Input::from_memory(encoded);
    std::vector<uint8_t> decoded_buffer;
    auto decoded_out = Output::from_memory(decoded_buffer);

    Coder::decode(input, decoded_out, DMS_MAX, 0);
    std::string decoded(decoded_buffer.begin(), decoded_buffer.end());

    ASSERT_EQ(decoded, "xyxaybxa!xa!?");

}

TEST(Lzw, LzwBitCoder) {
    using Coder = tudocomp::lzw::LzwBitCoder;

    Env env;
    std::vector<uint8_t> encoded_buffer;
    Output encoded_out = Output::from_memory(encoded_buffer);

    {
        Coder coder(env, encoded_out);

        coder.encode_fact('x');
        coder.encode_fact('y');
        coder.encode_fact('x');
        coder.encode_fact('a');
        coder.encode_fact('y');
        coder.encode_fact('b');
        coder.encode_fact(258);
        coder.encode_fact('!');
        coder.encode_fact(262);
        coder.encode_fact('?');
    }

    auto encoded = pack_integers({
        'x',9,
        'y',9,
        'x',9,
        'a',9,
        'y',9,
        'b',9,
        258,9,
        '!',9,
        262,9,
        '?',9,
    });
    ASSERT_EQ(encoded_buffer, encoded);

    auto input = Input::from_memory(encoded);
    std::vector<uint8_t> decoded_buffer;
    auto decoded_out = Output::from_memory(decoded_buffer);

    Coder::decode(input, decoded_out, DMS_MAX, 0);
    std::string decoded(decoded_buffer.begin(), decoded_buffer.end());

    ASSERT_EQ(decoded, "xyxaybxa!xa!?");

}

TEST(Lzw, LzwCompressor) {
    using Coder = tudocomp::lzw::LzwDebugCoder;
    using Compressor = tudocomp::lzw::LzwCompressor<Coder>;

    Env env;
    boost::string_ref input_str = "xyxaybxa!xa!?";

    std::vector<uint8_t> encoded_buffer;
    std::vector<uint8_t> decoded_buffer;
    Compressor compressor(env);

    {
        auto input = Input::from_memory(input_str);
        auto out = Output::from_memory(encoded_buffer);
        compressor.compress(input, out);
    }

    auto encoded = std::string(encoded_buffer.begin(), encoded_buffer.end());
    ASSERT_EQ(encoded, "'x','y','x','a','y','b',258,'!',262,'?',");

    {
        auto input = Input::from_memory(encoded_buffer);
        auto out = Output::from_memory(decoded_buffer);
        compressor.decompress(input, out);
    }

    ASSERT_EQ(decoded_buffer, std::vector<uint8_t>(input_str.begin(), input_str.end()));

}
