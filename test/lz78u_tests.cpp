
#include "tudocomp/tudocomp.hpp"
#include "test/util.hpp"
#include <gtest/gtest.h>

struct InputOutput {
    View in;
    View out;
};

std::ostream& operator<<(std::ostream& os, const InputOutput& v) {
    return os << v.in << " : " << v.out;
}

using namespace tdc;

TEST(Lz78U, roundtrip1) {
    test::roundtrip<LZ78UCompressor<ASCIICoder, ASCIICoder>>(
        "aaababaaabaababa"_v,
        "0:a\0""1:a\0""0:ba\0""3:a\0""1:ba\0""5:ba\0""\0"_v
    );
}

TEST(Lz78U, roundtrip2) {
    test::roundtrip<LZ78UCompressor<ASCIICoder, ASCIICoder>>(
        "abcdebcdeabc"_v,
        "0:abc\0""0:de\0""0:bc\0""2:a\0""3:\0""\0"_v
    );
}
/*
TEST(Lz78U, roundtrip3) {
    test::roundtrip<LZ78UCompressor<ASCIICoder, lz78u::StringCoder<HuffmanCoder>>>(
        "aaababaaabaababa"_v,
        ""_v
    );
}

TEST(Lz78U, roundtrip4) {
    test::roundtrip<LZ78UCompressor<ASCIICoder, lz78u::StringCoder<HuffmanCoder>>>(
        "abcdebcdeabc"_v,
        ""_v
    );
}*/
