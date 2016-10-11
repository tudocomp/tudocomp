#include <cstdint>
#include <iostream>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "test_util.hpp"
#include "tudocomp_test_util.hpp"

#include <tudocomp/lzw/LzwCompressor.hpp>
#include <tudocomp/lzw/LzwBitCoder.hpp>
#include <tudocomp/lzw/LzwDebugCoder.hpp>

using lz78_dictionary::DMS_MAX;
using tdc::string_ref;
using tdc::create_algo;

TEST(Lzw, compress) {
    {
        using Coder = tdc::lzw::LzwDebugCoder;
        using Compressor = tdc::lzw::LzwCompressor<Coder>;

        auto encoded = test::compress<Compressor>("LZWLZ78LZ77LZCLZMWLZAP");
        ASSERT_EQ(encoded.str,
            "'L','Z','W',256,'7','8',259,'7',256,'C',256,'M',258,'Z','A','P',");
        encoded.assert_decompress();
    }
    {
        using Coder = tdc::lzw::LzwDebugCoder;
        using Compressor = tdc::lzw::LzwCompressor<Coder>;

        auto encoded = test::compress<Compressor>("abcdebcdeabcd");
        ASSERT_EQ(encoded.str, "'a','b','c','d','e',257,259,256,258,");
        encoded.assert_decompress();
    }
    {
        using Coder = tdc::lzw::LzwDebugCoder;
        using Compressor = tdc::lzw::LzwCompressor<Coder>;

        auto encoded = test::compress<Compressor>("abcdebcdeabc");
        ASSERT_EQ(encoded.str, "'a','b','c','d','e',257,259,256,'c',");
        encoded.assert_decompress();
    }
    {
        using Coder = tdc::lzw::LzwDebugCoder;
        using Compressor = tdc::lzw::LzwCompressor<Coder>;

        auto encoded = test::compress<Compressor>("xyxaybxa!xa!?");
        ASSERT_EQ(encoded.str, "'x','y','x','a','y','b',258,'!',262,'?',");
        encoded.assert_decompress();
    }
    {
        using Coder = tdc::lzw::LzwBitCoder;
        using Compressor = tdc::lzw::LzwCompressor<Coder>;

        auto encoded = test::compress<Compressor>("abcdebcdeabc");
        ASSERT_EQ(encoded.bytes, (pack_integers({
            'a', 9,
            'b', 9,
            'c', 9,
            'd', 9,
            'e', 9,
            257, 9,
            259, 9,
            256, 9,
            'c', 9,
        })));
        encoded.assert_decompress();
    }
}

TEST(Lzw, LzwDebugCoder) {
    using Coder = tdc::lzw::LzwDebugCoder;

    std::vector<uint8_t> encoded_buffer;
    Output encoded_out = Output::from_memory(encoded_buffer);

    {
        auto coder = create_algo<Coder, Output&>("", encoded_out);

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
    using Coder = tdc::lzw::LzwBitCoder;

    std::vector<uint8_t> encoded_buffer;
    Output encoded_out = Output::from_memory(encoded_buffer);

    {
        auto coder = create_algo<Coder, Output&>("", encoded_out);

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
    using Coder = tdc::lzw::LzwDebugCoder;
    using Compressor = tdc::lzw::LzwCompressor<Coder>;

    string_ref input_str = "xyxaybxa!xa!?";

    std::vector<uint8_t> encoded_buffer;
    std::vector<uint8_t> decoded_buffer;

    auto compressor = create_algo<Compressor>();

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
