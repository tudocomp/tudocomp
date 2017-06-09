#include <gtest/gtest.h>

#include <tudocomp/Generator.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/io.hpp>

#include <tudocomp/generators/FibonacciGenerator.hpp>
#include <tudocomp/generators/ThueMorseGenerator.hpp>

#include <tudocomp/coders/ASCIICoder.hpp>
#include <tudocomp/coders/EliasDeltaCoder.hpp>
#include <tudocomp/coders/EliasGammaCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/coders/SLECoder.hpp>
#include <tudocomp/coders/ArithmeticCoder.hpp>
#include <tudocomp/coders/TernaryCoder.hpp>
#include <tudocomp/coders/AdaptiveHuffmanCoder.hpp>

using namespace tdc;

#define DUMMY_LITERALS ViewLiterals("dummy")

template<typename coder_t>
void test_mt() {
    // Don't encode anything
    std::stringstream ss;
    {
        Output out(ss);
        typename coder_t::Encoder coder(create_env(coder_t::meta()), out, DUMMY_LITERALS);
    }

    // Expect nothing to be decoded
    std::string result = ss.str();
    {
        Input in(result);
        typename coder_t::Decoder decoder(create_env(coder_t::meta()), in);
        ASSERT_TRUE(decoder.eof());
    }
}

template<typename coder_t>
void test_bits() {
    // Generate a thue morse word and use it as test subject
    const std::string word = ThueMorseGenerator::generate(16);

    // Encode thue morse bit vector
    std::stringstream ss;
    {
        Output out(ss);
        typename coder_t::Encoder coder(create_env(coder_t::meta()), out, DUMMY_LITERALS);

        for(char c : word) coder.encode(c != '0', bit_r);
    }

    // Decode bit vector
    std::string result = ss.str();
    {
        Input in(result);
        typename coder_t::Decoder decoder(create_env(coder_t::meta()), in);

        size_t i = 0;
        while(!decoder.eof()) {
            ASSERT_EQ(word[i++] != '0', decoder.template decode<bool>(bit_r)) << "i=" << i;
        }

        ASSERT_EQ(word.length(), i);
    }
}

template<typename coder_t>
void test_int(size_t n = 93) {
    // Encode Fibonacci series using fixed and dynamic ranges
    MinDistributedRange min_r(1, n); const size_t mod = n / 9;

    std::stringstream ss;
    {
        Output out(ss);
        typename coder_t::Encoder coder(create_env(coder_t::meta()), out, DUMMY_LITERALS);

        uint64_t a = 0, b = 1, t;
        for(size_t i = 0; i < n; i++) {
            DCHECK_LE(a, b);
            coder.encode(b, size_r);
            coder.encode(b, Range(b));
            coder.encode(b, Range(a, b));
            coder.encode(1 + (b % mod), min_r);
            t = b; b += a; a = t;
        }
    }

    // Decode
    std::string result = ss.str();
    {
        Input in(result);
        typename coder_t::Decoder decoder(create_env(coder_t::meta()), in);

        size_t i = 0;
        uint64_t a = 0, b = 1, t;
        while(!decoder.eof()) {
            ASSERT_EQ(b, decoder.template decode<uint64_t>(size_r))
                << "a=" << a << ", b=" << b;
            ASSERT_EQ(b, decoder.template decode<uint64_t>(Range(b)))
                << "a=" << a << ", b=" << b;
            ASSERT_EQ(b, decoder.template decode<uint64_t>(Range(a, b)))
                << "a=" << a << ", b=" << b;
            ASSERT_EQ(1 + (b % mod), decoder.template decode<size_t>(min_r))
                << "b=" << b;

            t = b; b += a; a = t; ++i;
        }

        ASSERT_EQ(n, i);
    }
}

template<typename coder_t>
void test_str() {
    // Generate a fibonacci word and use it as test subject
    const std::string word = FibonacciGenerator::generate(24);

    // Encode string
    std::stringstream ss;
    {
        Output out(ss);
        typename coder_t::Encoder coder(create_env(coder_t::meta()), out, ViewLiterals(word));

        for(char c : word) coder.encode(c, literal_r);
    }

    // Decode
    std::string result = ss.str();
    {
        Input in(result);
        typename coder_t::Decoder decoder(create_env(coder_t::meta()), in);

        size_t i = 0;
        while(!decoder.eof()) {
            ASSERT_EQ(word[i++], decoder.template decode<uliteral_t>(literal_r));
        }

        ASSERT_EQ(word.length(), i);
    }
}

template<typename coder_t>
void test_mixed() {
    // Generate a fibonacci word and use it as test subject
    const std::string word = FibonacciGenerator::generate(24);
    Range atoz_r('a', 'z');
    MinDistributedRange atoz_min_r('a', 'z');

    // Encode string
    std::stringstream ss;
    {
        Output out(ss);
        typename coder_t::Encoder coder(create_env(coder_t::meta()), out, ViewLiterals(word));

        for(size_t i = 0; i < word.length(); i++) {
            coder.encode(word[i] == 'a', bit_r);
            coder.encode(i, size_r);
            //coder.encode(word[i], literal_r);
            coder.encode(size_t(word[i]), atoz_min_r);
            coder.encode(size_t(word[i]), atoz_r);
        }
    }

    // Decode
    std::string result = ss.str();
    {
        Input in(result);
        typename coder_t::Decoder decoder(create_env(coder_t::meta()), in);

        size_t i = 0;
        while(!decoder.eof()) {
            ASSERT_EQ(word[i] == 'a', decoder.template decode<bool>(bit_r));
            ASSERT_EQ(i, decoder.template decode<size_t>(size_r));
            //ASSERT_EQ(word[i], decoder.template decode<uliteral_t>(literal_r));
            ASSERT_EQ(size_t(word[i]), decoder.template decode<size_t>(atoz_min_r));
            ASSERT_EQ(size_t(word[i]), decoder.template decode<size_t>(atoz_r));
            ++i;
        }

        ASSERT_EQ(word.length(), i);
    }
}

TEST(coder, ascii_mt) { test_mt<ASCIICoder>(); }
TEST(coder, ascii_bits) { test_bits<ASCIICoder>(); }
TEST(coder, ascii_int) { test_int<ASCIICoder>(); }
TEST(coder, ascii_str) { test_str<ASCIICoder>(); }
TEST(coder, ascii_mixed) { test_mixed<ASCIICoder>(); }

TEST(coder, sle_mt) { test_mt<SLECoder>(); }
TEST(coder, sle_bits) { test_bits<SLECoder>(); }
TEST(coder, sle_int) { test_int<SLECoder>(); }
TEST(coder, sle_str) { test_str<SLECoder>(); }
TEST(coder, sle_mixed) { test_mixed<SLECoder>(); }

TEST(coder, delta_mt) { test_mt<EliasDeltaCoder>(); }
TEST(coder, delta_bits) { test_bits<EliasDeltaCoder>(); }
TEST(coder, delta_int) { test_int<EliasDeltaCoder>(); }
TEST(coder, delta_str) { test_str<EliasDeltaCoder>(); }
TEST(coder, delta_mixed) { test_mixed<EliasDeltaCoder>(); }

TEST(coder, gamma_mt) { test_mt<EliasGammaCoder>(); }
TEST(coder, gamma_bits) { test_bits<EliasGammaCoder>(); }
TEST(coder, gamma_int) { test_int<EliasDeltaCoder>(); }
TEST(coder, gamma_str) { test_str<EliasDeltaCoder>(); }
TEST(coder, gamma_mixed) { test_mixed<EliasDeltaCoder>(); }

TEST(coder, huff_mt) { test_mt<HuffmanCoder>(); }
TEST(coder, huff_bits) { test_bits<HuffmanCoder>(); }
TEST(coder, huff_int) { test_int<HuffmanCoder>(); }
TEST(coder, huff_str) { test_str<HuffmanCoder>(); }
TEST(coder, huff_mixed) { test_mixed<HuffmanCoder>(); }

TEST(coder, arithm_mt) { test_mt<ArithmeticCoder>(); }
TEST(coder, arithm_bits) { test_bits<ArithmeticCoder>(); }
TEST(coder, arithm_int) { test_int<ArithmeticCoder>(); }
TEST(coder, arithm_str) { test_str<ArithmeticCoder>(); }
TEST(coder, arithm_mixed) { test_mixed<ArithmeticCoder>(); }

TEST(coder, ternary_mt) { test_mt<TernaryCoder>(); }
TEST(coder, ternary_bits) { test_bits<TernaryCoder>(); }
TEST(coder, ternary_int) { test_int<TernaryCoder>(); }
TEST(coder, ternary_str) { test_str<TernaryCoder>(); }
TEST(coder, ternary_mixed) { test_mixed<TernaryCoder>(); }

TEST(coder, adaphuff_mt) { test_mt<AdaptiveHuffmanCoder>(); }
TEST(coder, adaphuff_bits) { test_bits<AdaptiveHuffmanCoder>(); }
TEST(coder, adaphuff_int) { test_int<AdaptiveHuffmanCoder>(); }
TEST(coder, adaphuff_str) { test_str<AdaptiveHuffmanCoder>(); }
TEST(coder, adaphuff_mixed) { test_mixed<AdaptiveHuffmanCoder>(); }
