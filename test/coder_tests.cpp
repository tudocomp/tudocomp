#include <gtest/gtest.h>

#include <tudocomp/Generator.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/io.hpp>

#include <tudocomp/generators/FibonacciGenerator.hpp>
#include <tudocomp/generators/ThueMorseGenerator.hpp>

#include <tudocomp/coders/ASCIICoder.hpp>
#include <tudocomp/coders/Code2Coder.hpp>
#include <tudocomp/coders/EliasDeltaCoder.hpp>
#include <tudocomp/coders/EliasGammaCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>

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
    Range atoz_r = Range('a', 'z');

    // Encode string
    std::stringstream ss;
    {
        Output out(ss);
        typename coder_t::Encoder coder(create_env(coder_t::meta()), out, ViewLiterals(word));

        for(size_t i = 0; i < word.length(); i++) {
            coder.encode(word[i] == 'a', bit_r);
            coder.encode(i, size_r);
            coder.encode(word[i], literal_r);
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
            ASSERT_EQ(word[i], decoder.template decode<uliteral_t>(literal_r));
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

TEST(coder, code2_mt) { test_mt<Code2Coder>(); }
TEST(coder, code2_bits) { test_bits<Code2Coder>(); }
TEST(coder, code2_int) { test_int<Code2Coder>(); }
TEST(coder, code2_str) { test_str<Code2Coder>(); }
TEST(coder, code2_mixed) { test_mixed<Code2Coder>(); }

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
