#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"

#include "tudocomp.h"
#include "lz78.h"

using namespace lz78;

TEST(LZ78, compress) {
    // TODO:
    /*CompressorTest<LZ78Compressor>()
        .input("abaaabab")
        .threshold(0)
        .expected_rules(Rules {
            // (0,0)a(0,0)b(0,1)a(0,1)b(4,2)
            {0, 0, 0}, {1, 0, 0}, {2, 0, 1}, {4, 0, 1}, {6, 4, 2}
        })
        .run();
    CompressorTest<LZ78Compressor>()
        .input("abcdebcdeabc")
        .threshold(1)
        .expected_rules(Rules {
            {5, 1, 1}, {7, 3, 1}, {9, 0, 1}, {11, 2, 1}
        })
        .run();
    CompressorTest<LZ78Compressor>()
        .input("ananas")
        .threshold(0)
        .expected_rules(Rules {
            // (0,a)(0,n)(1,n)(1,s)
            {0, 0, 0}, {1, 0, 0}, {2, 0, 1}, {4, 0, 1}
        })
        .run();*/
}
