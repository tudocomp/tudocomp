#include "test/util.hpp"
#include <gtest/gtest.h>

#include <tudocomp/tudocomp.hpp>

struct InputOutput {
    View in;
    View out;
};

std::ostream& operator<<(std::ostream& os, const InputOutput& v) {
    return os << v.in << " : " << v.out;
}

using namespace tdc;
using namespace lz78u;

TEST(Lz78U, roundtrip1) {
    test::roundtrip<LZ78UCompressor<StreamingStrategy<ASCIICoder>, ASCIICoder>>(
        "aaababaaabaababa"_v,
        "0:a\0""1:a\0""0:ba\0""3:a\0""1:ba\0""5:ba\0""\0"_v
    );
}

TEST(Lz78U, roundtrip2) {
    test::roundtrip<LZ78UCompressor<StreamingStrategy<ASCIICoder>, ASCIICoder>>(
        "abcdebcdeabc"_v,
        "0:abc\0""0:de\0""0:bc\0""2:a\0""3:\0""\0"_v
    );
}

TEST(Lz78U, roundtrip3) {
    test::roundtrip<LZ78UCompressor<BufferingStrategy<HuffmanCoder>, ASCIICoder>>(
        "aaababaaabaababa"_v,
        // a0a0ba0a0ba0ba0
        // aaaaaa 000000 bbb
        //
        test::pack_integers({
            0b01, 2,
            0b1,  1,
            '0',  8,
            ':',  8,
        })
    );
}

TEST(Lz78U, roundtrip4) {
    test::roundtrip<LZ78UCompressor<BufferingStrategy<HuffmanCoder>, ASCIICoder>>(
        "abcdebcdeabc"_v,
        "a"_v
    );
}
