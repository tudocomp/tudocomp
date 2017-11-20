#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include <tudocomp/def.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/util/View.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>

#include <tudocomp/compressors/NoopCompressor.hpp>
#include <tudocomp/compressors/RunLengthEncoder.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>

#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp_driver/Registry.hpp>
#include <tudocomp/compressors/ChainCompressor.hpp>
#include <tudocomp/compressors/DividingCompressor.hpp>

#include "test/util.hpp"

using namespace tdc;
using namespace tdc_algorithms;

class NoopEscapingCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m ("compressor", "noop_null", "");
        m.input_restrictions(io::InputRestrictions({0}, true));
        m.option("mode").dynamic("view");
        m.option("debug").dynamic("false");
        return m;
    }

    inline NoopEscapingCompressor(Env&& env):
        Compressor(std::move(env)) {}


    inline virtual void compress(Input& i, Output& o) override final {
        auto os = o.as_stream();

        if (env().option("mode").as_string() == "stream") {
            auto is = i.as_stream();
            if (env().option("debug").as_bool()) {
                std::stringstream ss;
                ss << is.rdbuf();
                std::string txt = ss.str();
                DLOG(INFO) << vec_to_debug_string(txt);
                os << txt;
            } else {
                os << is.rdbuf();
            }
        } else {
            auto iv = i.as_view();
            if (env().option("debug").as_bool()) {
                DLOG(INFO) << vec_to_debug_string(iv);
                os << iv;
            } else {
                os << iv;
            }
        }
    }

    inline virtual void decompress(Input& i, Output& o) override final {
        auto os = o.as_stream();

        if (env().option("mode").as_string() == "stream") {
            auto is = i.as_stream();
            if (env().option("debug").as_bool()) {
                std::stringstream ss;
                ss << is.rdbuf();
                std::string txt = ss.str();
                DLOG(INFO) << vec_to_debug_string(txt);
                os << txt;
            } else {
                os << is.rdbuf();
            }
        } else {
            auto iv = i.as_view();
            if (env().option("debug").as_bool()) {
                DLOG(INFO) << vec_to_debug_string(iv);
                os << iv;
            } else {
                os << iv;
            }
        }
    }
};

TEST(A, a) {
    FLAGS_logtostderr = 0;
    COMPRESSOR_REGISTRY.register_algorithm<NoopEscapingCompressor>();
}

// TEST(Chain, test0) { // TODO: has to be adapted for the easyrle
//     test::roundtrip<RunLengthEncoder<ASCIICoder>>("aaaaaabaaaaaabaaaaaabaaaaaab",
//                                      "aa14:baa14:baa14:baa14:b\0"_v,
//                                      R"(
//                                          ascii
//                                     )", COMPRESSOR_REGISTRY);
// }

// TEST(Chain, test1) {
//     test::roundtrip<ChainCompressor>("aaaaaabaaaaaabaaaaaabaaaaaab",
//                                      "aa14:baa14:baa14:baa14:b\0"_v,
//                                      R"(
//                                         noop,
//                                         rle(ascii),
//                                     )", COMPRESSOR_REGISTRY);
// }
//
// TEST(Chain, test2) {
//     test::roundtrip<ChainCompressor>("aaaaaabaaaaaabaaaaaabaaaaaab",
//                                      "aa14:baa14:baa14:baa14:b\0"_v,
//                                      R"(
//                                         rle(ascii),
//                                         noop,
//                                     )", COMPRESSOR_REGISTRY);
// }
//
// TEST(Chain, test3) {
//     test::roundtrip<ChainCompressor>("aaaaaabaaaaaabaaaaaabaaaaaab",
//                                      "97:97:49:52:58:98:256:258:260:262:259:261:257:266:0:\0"_v,
//                                      R"(
//                                         rle(ascii),
//                                         lzw(ascii),
//                                     )", COMPRESSOR_REGISTRY);
// }
//
// TEST(Chain, test4) {
//     test::roundtrip<ChainCompressor>("aaaaaabaaaaaabaaaaaabaaaaaab",
//                                      "97:97:49:52:58:98:256:258:260:262:259:261:257:266:0:\0"_v,
//                                      R"(
//                                         noop,
//                                         chain(
//                                             rle(ascii),
//                                             lzw(ascii)
//                                         )
//                                     )", COMPRESSOR_REGISTRY);
// }

const View CHAIN_STRING = "abcd"_v;
const View CHAIN_STRING_NULL = "abcd\0"_v;

TEST(ChainNull, stream_noop) {
    test::roundtrip_ex<NoopCompressor>(CHAIN_STRING, CHAIN_STRING,
        R"('stream', true)", COMPRESSOR_REGISTRY);
}

TEST(ChainNull, view_noop) {
    test::roundtrip_ex<NoopCompressor>(CHAIN_STRING, CHAIN_STRING,
        R"('view', true)", COMPRESSOR_REGISTRY);
}

TEST(ChainNull, view_noop_null) {
    test::roundtrip_ex<NoopEscapingCompressor>(CHAIN_STRING, CHAIN_STRING_NULL,
        R"('view', true)", COMPRESSOR_REGISTRY);
}

TEST(ChainNull, stream_chain_noop_noop) {
    test::roundtrip_ex<ChainCompressor>(CHAIN_STRING, CHAIN_STRING,
        R"(noop('stream', true), noop('stream', true))", COMPRESSOR_REGISTRY);
}

TEST(ChainNull, view_chain_noop_noop) {
    test::roundtrip_ex<ChainCompressor>(CHAIN_STRING, CHAIN_STRING,
        R"(noop('view', true), noop('view', true))", COMPRESSOR_REGISTRY);
}

TEST(ChainNull, view_chain_noop_null_noop) {
    test::roundtrip_ex<ChainCompressor>(CHAIN_STRING, CHAIN_STRING_NULL,
        R"(noop_null('view', true), noop('view', true))", COMPRESSOR_REGISTRY);
}

TEST(ChainNull, view_chain_noop_noop_null) {
    test::roundtrip_ex<ChainCompressor>(CHAIN_STRING, CHAIN_STRING_NULL,
        R"(noop('view', true), noop_null('view', true))", COMPRESSOR_REGISTRY);
}

TEST(NoopCompressor, test) {
    test::roundtrip_ex<NoopCompressor>("abcd", "abcd");
    test::roundtrip_ex<NoopCompressor>("äüö", "äüö");
    test::roundtrip_ex<NoopCompressor>("\0\xff"_v, "\0\xff"_v);
    test::roundtrip_ex<NoopCompressor>("\xff\0"_v, "\xff\0"_v);
}

TEST(NoopEscapingCompressor, noop) {
    test::roundtrip_ex<NoopEscapingCompressor>("abcd", "abcd\0"_v);
}

TEST(NoopEscapingCompressor, escaping) {
    test::roundtrip_ex<NoopEscapingCompressor>("ab\xff!"_v, "ab\xff\xff!\0"_v);
    test::roundtrip_ex<NoopEscapingCompressor>("ab\x00!"_v, "ab\xff\xfe!\0"_v);
    test::roundtrip_ex<NoopEscapingCompressor>("ab\xfe!"_v, "ab\xfe!\0"_v);

    std::vector<uint8_t> a {
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
        16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
        32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
        48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
        64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
        80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
        96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
        144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    };

    std::vector<uint8_t> b {
        255, 254,
              1,   2,  3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
        16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
        32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
        48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
        64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
        80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
        96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
        144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
        255, 255,
        0
    };

    test::roundtrip_ex<NoopEscapingCompressor>(View(a), View(b));
}

TEST(Dividing, test_division) {
    test::roundtrip_ex<DividingCompressor<DivisionDividingStrategy>>(
        "569874523695214569874523196875"_v, ""_v,
        R"(strategy=division(2), compressor=lz78(ascii))", COMPRESSOR_REGISTRY);
}

TEST(Dividing, test_blocked) {
    test::roundtrip_ex<DividingCompressor<BlockedDividingStrategy>>(
        "569874523695214569874523196875"_v, ""_v,
        R"(strategy=blocked(10), compressor=lz78(ascii))", COMPRESSOR_REGISTRY);
}
