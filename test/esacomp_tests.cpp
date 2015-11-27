#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"

#include "tudocomp.h"
#include "lz77rule.h"
#include "esa_compressor.h"
#include "code0.h"
#include "code1.h"
#include "code2.h"

using namespace lz77rule;
using namespace esacomp;

TEST(ESACompressor, compress) {
    CompressorTest<ESACompressor<>>()
        .input("abcdebcdeabc")
        .threshold(2)
        .expected_rules(Rules { {1, 5, 4}, {5, 10, 2} })
        .run();
}

TEST(ESACompressor, compress_max_lcp_heap) {
    CompressorTest<ESACompressor<MaxLCPHeap>>()
        .input("abcdebcdeabc")
        .threshold(2)
        .expected_rules(Rules { {1, 5, 4}, {5, 10, 2} })
        .run();
}

TEST(ESACompressor, compress_max_lcp_ssl) {
    CompressorTest<ESACompressor<MaxLCPSortedSuffixList>>()
        .input("abcdebcdeabc")
        .threshold(2)
        .expected_rules(Rules { {1, 5, 4}, {5, 10, 2} })
        .run();
}

TEST(Code0Coder, basic) {
    auto test = CoderTest<Code0Coder>()
        .input("abcdebcdeabc");

    test.rules(Rules { {1, 5, 4}, {5, 10, 2} })
        .expected_output("12:a{6,4}{11,2}deabc")
        .run();

    test.rules(Rules { {5, 1, 4}, {9, 0, 3} })
        .expected_output("12:abcde{2,4}{1,3}")
        .run();
}

TEST(Code0Coder, emptyRules) {
    CoderTest<Code0Coder>()
        .input("abcdebcdeabc")
        .rules(Rules {})
        .expected_output("12:abcdebcdeabc")
        .run();
}

TEST(Code0Coder, emptyInput) {
    CoderTest<Code0Coder>()
        .input("")
        .rules(Rules {})
        .expected_output("0:")
        .run();
}

TEST(Code1Coder, basic) {
    auto test = CoderTest<Code1Coder>()
        .input("abcdebcdeabc");

    test.rules(Rules { {1, 5, 4}, {5, 10, 2} })
        .expected_output(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 1, 5, 4, 'b', 'c', 'd', 'e', 'a', 'b', 'c'
        })
        .run();

    test.rules(Rules { {5, 1, 4}, {9, 0, 3} })
        .expected_output(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 'b', 'c', 'd', 'e', 1, 1, 4, 'a', 'b', 'c'
        })
        .run();
}

TEST(Code1Coder, emptyRules) {
    auto test = CoderTest<Code1Coder>()
        .input("abcdebcdeabc");

    test.rules(Rules { })
        .expected_output(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 'b', 'c', 'd', 'e', 'b', 'c', 'd', 'e', 'a', 'b', 'c'
        })
        .run();
}

TEST(Code1Coder, emptyInput) {
    auto test = CoderTest<Code1Coder>()
        .input("");

    test.rules(Rules { })
        .expected_output(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 0,
            17,
        })
        .run();
}

TEST(Code2Coder, basic) {
    auto test = CoderTest<Code2Coder>()
        .input("abcdebcdeabc");

    test.rules(Rules {
            {1, 5, 4}, {5, 10, 2}
        })
        .expected_output(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12, // length
            0, 0, 0, 2, // threshold
            3, // bits per symbol
            3, // bits per sublen
            4, // bits per ref
            0, 7, // alphabet count
            // alphabet
            0, 'a',
            0, 'b',
            0, 'c',
            0, 'd',
            0, 'e',
            248, 0,
            248, 1,
            0, 2, // phrase count
            // phrases
            'd', 'e', 'a',
            'e', 'a', 'b',
            // encoded text
            0b00001010, 0b10001011, 0b01000000, 0b01010001, 0b00100000
        })
        .run();

    test.rules(Rules {
            {5, 1, 4}, {9, 0, 3}
        })
        .expected_output(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12,
            0, 0, 0, 3,
            3,
            3,
            1,
            0, 7,
            0, 97,
            0, 98,
            0, 99,
            0, 100,
            0, 101,
            248, 0,
            248, 1,
            0, 2,
            97, 98, 99,
            98, 99, 100,
            0b01010011, 0b01001100, 0b00110000, 0b00000000
            //0b01010011, 0b01001100, 0b01010000, 0b01000000 with threshold 2
        })
        .run();
}

TEST(Code2Coder, emptyRules) {
    auto test = CoderTest<Code2Coder>()
        .input("abcdebcdeabc");

    test.rules(Rules {})
        .expected_output(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12,
            0, 0, 0, 0,
            4,
            1,
            1,
            0, 12,
            0, 98,
            0, 99,
            0, 97,
            0, 100,
            0, 101,
            248, 0,
            248, 1,
            248, 2,
            248, 3,
            248, 4,
            248, 5,
            248, 6,
            0, 7,
            'b', 'c', 'd',
            'c', 'd', 'e',
            'a', 'b', 'c',
            'd', 'e', 'a',
            'd', 'e', 'b',
            'e', 'a', 'b',
            'e', 'b', 'c',
            0b00011100, 0b10010001, 0b10011001, 0b00010100
            // 0 00111 0 01001 0 00110 0 110 0 100 0 101 00
        })
        .run();
}

TEST(Code2Coder, emptyInput) {
    auto test = CoderTest<Code2Coder>()
        .input("");

    test.rules(Rules {})
        .expected_output(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0,
            0,
            1,
            1,
            0, 0,
            0, 0,
        })
        .run();
}

TEST(Code0Decoder, basic) {
    auto test = DecoderTest<Code0Coder>()
        .expected_output("abcdebcdeabc");

    test.input("12:a{6,4}{11,2}deabc")
        .run();

    test.input("12:abcde{2,4}{1,3}")
        .run();

    test.input("12:abcde{2,2}dea{6,2}")
        .run();

    DecoderTest<Code0Coder>()
        .input("14:{7,3}{10,3}abcdef{3,2}")
        .expected_output("abcdefabcdefcd")
        .run();
}

TEST(Code1Decoder, basic) {
    auto test = DecoderTest<Code1Coder>()
        .expected_output("abcdebcdeabc");

    test.input(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 1, 5, 4, 1, 10, 2, 'd', 'e', 'a', 'b', 'c'
        })
        .run();

    test.input(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 'b', 'c', 'd', 'e', 1, 1, 4, 1, 0, 3
        })
        .run();
}

TEST(Code2Decoder, basic) {
    auto test = DecoderTest<Code2Coder>()
        .expected_output("abcdebcdeabc");

    test.input(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12, // length
            0, 0, 0, 2, // threshold
            3, // bits per symbol
            2, // bits per sublen
            4, // bits per ref
            0, 7, // alphabet count
            // alphabet
            0, 'a',
            0, 'b',
            0, 'c',
            0, 'd',
            0, 'e',
            248, 0,
            248, 1,
            0, 2, // phrase count
            // phrases
            'd', 'e', 'a',
            'e', 'a', 'b',
            // encoded text
            0b00001010, 0b10001011, 0b01000000, 0b01010001, 0b00100000
        })
        .run();

    test.input(std::vector<uint8_t> {
            0, 0, 0, 0, 0, 0, 0, 12,
            0, 0, 0, 2,
            3,
            2,
            1,
            0, 7,
            0, 97,
            0, 98,
            0, 99,
            0, 100,
            0, 101,
            248, 0,
            248, 1,
            0, 2,
            97, 98, 99,
            98, 99, 100,
            0b01010011, 0b01001100, 0b01010000, 0b01000000
        })
        .run();
}

TEST(Roundtrip, ESACompressorMaxLCPSortedSuffixListCode0Coder) {
    test_roundtrip_batch(lz77roundtrip<ESACompressor<MaxLCPSortedSuffixList>, Code0Coder>);
}

TEST(Roundtrip, ESACompressorMaxLCPSortedSuffixListCode1Coder) {
    test_roundtrip_batch(lz77roundtrip<ESACompressor<MaxLCPSortedSuffixList>, Code1Coder>);
}

TEST(Roundtrip, ESACompressorMaxLCPSortedSuffixListCode2Coder) {
    test_roundtrip_batch(lz77roundtrip<ESACompressor<MaxLCPSortedSuffixList>, Code2Coder>);
}

TEST(Roundtrip, ESACompressorMaxLCPHeapCode0Coder) {
    test_roundtrip_batch(lz77roundtrip<ESACompressor<MaxLCPHeap>, Code0Coder>);
}

TEST(Roundtrip, ESACompressorMaxLCPHeapCode1Coder) {
    test_roundtrip_batch(lz77roundtrip<ESACompressor<MaxLCPHeap>, Code1Coder>);
}

TEST(Roundtrip, ESACompressorMaxLCPHeapCode2Coder) {
    test_roundtrip_batch(lz77roundtrip<ESACompressor<MaxLCPHeap>, Code2Coder>);
}
