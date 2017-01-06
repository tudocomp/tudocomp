
#include "test/util.hpp"
#include <gtest/gtest.h>
#include <tudocomp/compressors/LZ78Compressor.hpp>
#include <tudocomp/compressors/LZWCompressor.hpp>
#include <tudocomp/compressors/lz78/BinaryTrie.hpp>
#include <tudocomp/compressors/lz78/TernaryTrie.hpp>
#include <tudocomp/compressors/lz78/CedarTrie.hpp>
#include <tudocomp/coders/BitCoder.hpp>

struct InputOutput {
    View in;
    View out;
};

std::ostream& operator<<(std::ostream& os, const InputOutput& v) {
    return os << v.in << " : " << v.out;
}

using namespace tdc;

class NotCedarLz78Compress: public ::testing::TestWithParam<InputOutput> {};
TEST_P(NotCedarLz78Compress, test) {
    auto c = create_algo<LZ78Compressor<ASCIICoder, lz78::BinaryTrie>>();
    test::TestInput i(GetParam().in, false);
    test::TestOutput o(false);
    c.compress(i, o);
    ASSERT_EQ(o.result(), GetParam().out);
}
INSTANTIATE_TEST_CASE_P(
    InputOutput, NotCedarLz78Compress, ::testing::Values(
        InputOutput { "aababcabcdabcde"_v, "0:a1:b2:c3:d4:e\0"_v },
        InputOutput { "aababcabcdabcdeabc"_v, "0:a1:b2:c3:d4:e2:c\0"_v },
        InputOutput { "\0\0b\0bc\0bcd\0bcde"_v, "0:\0""1:b2:c3:d4:e\0"_v },
        InputOutput { "\xfe\xfe""b\xfe""bc\xfe""bcd\xfe""bcde"_v, "0:\xfe""1:b2:c3:d4:e\0"_v },
        InputOutput { "\xff\xff""b\xff""bc\xff""bcd\xff""bcde"_v, "0:\xff""1:b2:c3:d4:e\0"_v }
    )
);


class CedarLz78Compress: public ::testing::TestWithParam<InputOutput> {};
TEST_P(CedarLz78Compress, test) {
    auto c = create_algo<LZ78Compressor<ASCIICoder, lz78::CedarTrie>>();
    test::TestInput i(GetParam().in, false);
    test::TestOutput o(false);
    c.compress(i, o);
    ASSERT_EQ(o.result(), GetParam().out);
}
INSTANTIATE_TEST_CASE_P(
    InputOutput, CedarLz78Compress, ::testing::Values(
        InputOutput { "aababcabcdabcde"_v, "0:a1:b2:c3:d4:e\0"_v },
        InputOutput { "aababcabcdabcdeabc"_v, "0:a1:b2:c3:d4:e2:c\0"_v },
        InputOutput { "\0\0b\0bc\0bcd\0bcde"_v, "0:\0""1:b2:c3:d4:e\0"_v },
        InputOutput { "\xfe\xfe""b\xfe""bc\xfe""bcd\xfe""bcde"_v, "0:\xfe""1:b2:c3:d4:e\0"_v },
        InputOutput { "\xff\xff""b\xff""bc\xff""bcd\xff""bcde"_v, "0:\xff""1:b2:c3:d4:e\0"_v }
    )
);

class NotCedarLzwCompress: public ::testing::TestWithParam<InputOutput> {};
TEST_P(NotCedarLzwCompress, test) {
    auto c = create_algo<LZWCompressor<ASCIICoder, lz78::BinaryTrie>>();
    test::TestInput i(GetParam().in, false);
    test::TestOutput o(false);
    c.compress(i, o);
    ASSERT_EQ(o.result(), GetParam().out);
}
INSTANTIATE_TEST_CASE_P(InputOutput,
                        NotCedarLzwCompress,
                        ::testing::Values(
                            InputOutput { "aaaaaa"_v, "97:256:257:\0"_v },
                            InputOutput { "aaaaaaa"_v, "97:256:257:97:\0"_v },
                            InputOutput { "a\0b"_v, "97:0:98:\0"_v },
                            InputOutput { "a\xfe""b"_v, "97:254:98:\0"_v },
                            InputOutput { "a\xff""b"_v, "97:255:98:\0"_v },
                            InputOutput { "\0\0\0"_v, "0:256:\0"_v },
                            InputOutput { "\xfe\xfe\xfe"_v, "254:256:\0"_v },
                            InputOutput { "\xff\xff\xff"_v, "255:256:\0"_v }
                        ));


class CedarLzwCompress: public ::testing::TestWithParam<InputOutput> {};
TEST_P(CedarLzwCompress, test) {
    auto c = create_algo<LZWCompressor<ASCIICoder, lz78::CedarTrie>>();
    test::TestInput i(GetParam().in, false);
    test::TestOutput o(false);
    c.compress(i, o);
    ASSERT_EQ(o.result(), GetParam().out);
}
INSTANTIATE_TEST_CASE_P(InputOutput,
                        CedarLzwCompress,
                        ::testing::Values(
                            InputOutput { "aaaaaa"_v, "97:256:257:\0"_v },
                            InputOutput { "aaaaaaa"_v, "97:256:257:97:\0"_v },
                            InputOutput { "a\0b"_v, "97:0:98:\0"_v },
                            InputOutput { "a\xfe""b"_v, "97:254:98:\0"_v },
                            InputOutput { "a\xff""b"_v, "97:255:98:\0"_v },
                            InputOutput { "\0\0\0"_v, "0:256:\0"_v },
                            InputOutput { "\xfe\xfe\xfe"_v, "254:256:\0"_v },
                            InputOutput { "\xff\xff\xff"_v, "255:256:\0"_v }
                        ));

/*
TEST(zcedar, base) {
    cedar::da<uint32_t> trie;

    for (uint16_t j = 0; j < 256; j++) {
        const char c = uint8_t(j);
        auto letter = &c;

        size_t from = 0;
        size_t pos = 0;

        auto i = j;

        //DLOG(INFO) << "i: " << i << ", from: " << from << ", pos: " << pos;
        trie.update(letter, from, pos, 1, i);
        DLOG(INFO) << "i: " << i << ", from: " << from << ", pos: " << pos;
    }

    for (uint16_t i = 0; i < 256; i++) {
        const char c = uint8_t(i);
        auto letter = &c;

        size_t from = 0;
        size_t pos = 0;


        auto r = trie.traverse(letter, from, pos, 1);
        DLOG(INFO) << "r: " << r << ", from: " << from << ", pos: " << pos
        << ", NO_PATH: " << int(r == lz78::CEDAR_NO_PATH)
        << ", NO_VALUE: " << int(r == lz78::CEDAR_NO_VALUE);
    }

    for (uint16_t i = 0; i < 256; i++) {
        const char c = uint8_t(i);
        auto letter = &c;

        size_t from = 0;
        size_t pos = 0;


        auto r = trie.traverse(letter, from, pos, 1);
        //if (r != lz78::CEDAR_NO_PATH && r !=  lz78::CEDAR_NO_VALUE)
        {
            const char c2 = 'a';
            auto letter2 = &c2;
            pos = 0;
            trie.update(letter2, from, pos, 1);
        }

        DLOG(INFO) << "    r: " << r << ", from: " << from << ", pos: " << pos
        << ", NO_PATH: " << int(r == lz78::CEDAR_NO_PATH)
        << ", NO_VALUE: " << int(r == lz78::CEDAR_NO_VALUE);
    }

}
*/
