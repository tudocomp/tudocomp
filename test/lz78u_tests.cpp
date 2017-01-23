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


TEST(Lz78U, roundtrip2) {
    test::roundtrip_binary<LZ78UCompressor<StreamingStrategy<ASCIICoder>, ASCIICoder>>(
        "abcdebcdeabc"_v, {
            '0', 8,
            ':', 8,
             1 , 1,
            'a', 8,
            'b', 8,
            'c', 8,
             0 , 8,

            '0', 8,
            ':', 8,
             1 , 1,
            'd', 8,
            'e', 8,
             0 , 8,

            '0', 8,
            ':', 8,
             1 , 1,
            'b', 8,
            'c', 8,
             0 , 8,

            '2', 8,
            ':', 8,
             1 , 1,
            'a', 8,
             0 , 8,

            '3', 8,
            ':', 8,
             1 , 1,
             0 , 8,

             5 , 3,
        }
    );
}

TEST(Lz78U, roundtrip1) {
    test::roundtrip_binary<LZ78UCompressor<StreamingStrategy<ASCIICoder>, ASCIICoder>>(
        "aaababaaabaababa"_v, {
            '0', 8,
            ':', 8,
             1 , 1,
            'a', 8,
             0 , 8,

            '1', 8,
            ':', 8,
             1 , 1,
            'a', 8,
             0 , 8,

            '0', 8,
            ':', 8,
             1 , 1,
            'b', 8,
            'a', 8,
             0 , 8,

            '3', 8,
            ':', 8,
             1 , 1,
            'a', 8,
             0 , 8,

            '1', 8,
            ':', 8,
             1 , 1,
            'b', 8,
            'a', 8,
             0 , 8,

            '5', 8,
            ':', 8,
             1 , 1,
            'b', 8,
            'a', 8,
             0 , 8,

             6 ,10,
        }
    );
}

TEST(Lz78U, roundtrip3) {
    test::roundtrip_binary<LZ78UCompressor<BufferingStrategy<HuffmanCoder>, ASCIICoder>>(
        "aaababaaabaababa"_v,
        // a0a0ba0a0ba0ba0
        // aaaaaa 000000 bbb
        //
        {
            0b1,        1,
            0b00000010, 8, // 2 codeword lengths
            0b00000001, 8, // 1 of 1
            0b00000010, 8, // 2 of 2
            0b00000011, 8, // 3 characters:
            '\0',       8, // => 0b1
            'a',        8, // => 0b00
            'b',        8, // => 0b01

            // 1. tuple
            '0',        8,
            ':',        8,
            0b1,        1,
            0b00,       2,
            0b1,        1,

            // 2. tuple
            '1',        8,
            ':',        8,
            0b1,        1,
            0b00,       2,
            0b1,        1,

            // 3. tuple
            '0',        8,
            ':',        8,
            0b1,        1,
            0b01,       2,
            0b00,       2,
            0b1,        1,

            // 4. tuple
            '3',        8,
            ':',        8,
            0b1,        1,
            0b00,       2,
            0b1,        1,

            // 5. tuple
            '1',        8,
            ':',        8,
            0b1,        1,
            0b01,       2,
            0b00,       2,
            0b1,        1,

            // 6. tuple
            '5',        8,
            ':',        8,
            0b1,        1,
            0b01,       2,
            0b00,       2,
            0b1,        1,

            // terminator
            0b000000111,9,
        }
    );
}
