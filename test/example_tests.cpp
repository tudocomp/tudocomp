// [/test/example_tests.cpp]

#include "gtest/gtest.h"

#include <tudocomp/example/ExampleCompressor.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include "tudocomp_test_util.h"

using tudocomp::ExampleCompressor;

TEST(example, test) {
    ASSERT_TRUE(true);
}

TEST(example, manual_compress) {
    auto compressor = tudocomp::create_algo<ExampleCompressor>();

    tudocomp::Input i("abcccccccde");

    std::vector<uint8_t> o_buf;
    tudocomp::Output o(o_buf);

    compressor.compress(i, o);

    std::string o_buf_s(o_buf.begin(), o_buf.end());
    ASSERT_EQ(o_buf_s, "abc%6%de");
}

TEST(example, manual_decompress) {
    auto compressor = tudocomp::create_algo<ExampleCompressor>();

    tudocomp::Input i("abc%6%de");

    std::vector<uint8_t> o_buf;
    tudocomp::Output o(o_buf);

    compressor.decompress(i, o);

    std::string o_buf_s(o_buf.begin(), o_buf.end());
    ASSERT_EQ(o_buf_s, "abcccccccde");
}

TEST(example, roundtrip1) {
    test::roundtrip<ExampleCompressor>("abcccccccde", "abc%6%de");
}

TEST(example, roundtrip2) {
    std::vector<uint8_t> v { 97, 98, 99, 37, 54, 37, 100, 101 };
    test::roundtrip<ExampleCompressor>("abcccccccde", v);
}

TEST(example, compress_stats_options) {
    auto compressor
        = tudocomp::create_algo<ExampleCompressor>(" '/', true ");

    tudocomp::Input i("abcccccccde");

    std::vector<uint8_t> o_buf;
    tudocomp::Output o(o_buf);

    compressor.compress(i, o);

    std::string o_buf_s(o_buf.begin(), o_buf.end());
    ASSERT_EQ(o_buf_s, "abc/6/de");

    std::cout << compressor.env().finish_stats().to_json() << "\n";

    test::roundtrip<ExampleCompressor>("abcccccccde",
                                       "abc-6-de",
                                       "escape_symbol = '-'");
    test::roundtrip<ExampleCompressor>("abcccccccde",
                                       "abc%6%de",
                                       "debug_sleep = true");
}

using tudocomp::TemplatedExampleCompressor;
using tudocomp::ExampleDebugCoder;
using tudocomp::ExampleBitCoder;

TEST(example, templated_debug) {
    test::roundtrip<TemplatedExampleCompressor<ExampleDebugCoder>>(
        "abbbbcccccccde", "abbbbc%6%de");

    test::roundtrip<TemplatedExampleCompressor<ExampleDebugCoder>>(
        "abbbbcccccccde", "abbbbc%6%de", "debug()");

    test::roundtrip<TemplatedExampleCompressor<ExampleDebugCoder>>(
        "abbbbcccccccde", "abbbbc-6-de", "debug(escape_symbol = '-')");
}

TEST(example, templated_bit) {
    test::roundtrip<TemplatedExampleCompressor<ExampleBitCoder>>(
        "abbbbcccccccde",
        std::vector<uint8_t> { 'a', 'b', 0xff, 3, 'c', 0xff, 6, 'd', 'e' });

    test::roundtrip<TemplatedExampleCompressor<ExampleBitCoder>>(
        "abbbbcccccccde",
        std::vector<uint8_t> { 'a', 'b', 0xff, 3, 'c', 0xff, 6, 'd', 'e' },
        "bit()");

    test::roundtrip<TemplatedExampleCompressor<ExampleBitCoder>>(
        "abbbbcccccccde",
        std::vector<uint8_t> { 'a', 'b', 0, 3, 'c', 0, 6, 'd', 'e' },
        "bit(escape_byte = '0')");
}
