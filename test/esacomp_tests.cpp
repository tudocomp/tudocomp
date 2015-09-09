#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"

#include "tudocomp.h"
#include "esa_compressor.h"
#include "code0.h"
#include "code1.h"
#include "code2.h"

using namespace esacomp;

TEST(ESACompressor, compress) {
    const Input input = input_from_string("abcdebcdeabc");
    ESACompressor<> compressor;
    Rules rules = compressor.compress(input, 2);
    Rules x { {1, 5, 4}, {5, 10, 2} };

    ASSERT_EQ(rules.size(), x.size());
    for (size_t i = 0; i < rules.size(); i++)
        ASSERT_EQ(rules[i], x[i]);
}

TEST(ESACompressor, compress_max_lcp_heap) {
    const Input input = input_from_string("abcdebcdeabc");
    ESACompressor<MaxLCPHeap> compressor;
    Rules rules = compressor.compress(input, 2);
    Rules x { {1, 5, 4}, {5, 10, 2} };

    ASSERT_EQ(rules.size(), x.size());
    for (size_t i = 0; i < rules.size(); i++)
        ASSERT_EQ(rules[i], x[i]);
}

TEST(ESACompressor, compress_max_lcp_ssl) {
    const Input input = input_from_string("abcdebcdeabc");
    ESACompressor<MaxLCPSortedSuffixList> compressor;
    Rules rules = compressor.compress(input, 2);
    Rules x { {1, 5, 4}, {5, 10, 2} };

    ASSERT_EQ(rules.size(), x.size());
    for (size_t i = 0; i < rules.size(); i++)
        ASSERT_EQ(rules[i], x[i]);
}

TEST(Code0Coder, basic) {
    const Input input = input_from_string("abcdebcdeabc");
    {
        Rules rules { {1, 5, 4}, {5, 10, 2} };
        std::string should_be("12:a{6,4}{11,2}deabc");

        Code0Coder coder;
        auto coded = ostream_to_string([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
    {
        Rules rules { {5, 1, 4}, {9, 0, 3} };
        std::string should_be("12:abcde{2,4}{1,3}");

        Code0Coder coder;
        auto coded = ostream_to_string([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
}

TEST(Code0Coder, emptyRules) {
    const Input input = input_from_string("abcdebcdeabc");
    Rules rules {  };
    std::string should_be("12:abcdebcdeabc");

    Code0Coder coder;
    auto coded = ostream_to_string([&] (std::ostream& out) {
        coder.code(rules, input, 2, out);
    });
    ASSERT_EQ(should_be, coded);
}

TEST(Code0Coder, emptyInput) {
    const Input input = input_from_string("");
    Rules rules {  };
    std::string should_be("0:");

    Code0Coder coder;
    auto coded = ostream_to_string([&] (std::ostream& out) {
        coder.code(rules, input, 2, out);
    });
    ASSERT_EQ(should_be, coded);
}

TEST(Code1Coder, basic) {
    const Input input = input_from_string("abcdebcdeabc");
    {
        Rules rules { {1, 5, 4}, {5, 10, 2} };
        std::vector<uint8_t> should_be {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 1, 5, 4, 1, 10, 2, 'd', 'e', 'a', 'b', 'c'
        };

        Code1Coder coder;
        auto coded = ostream_to_bytes([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
    {
        Rules rules { {5, 1, 4}, {9, 0, 3} };
        std::vector<uint8_t> should_be {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 'b', 'c', 'd', 'e', 1, 1, 4, 1, 0, 3
        };

        Code1Coder coder;
        auto coded = ostream_to_bytes([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
}

TEST(Code1Coder, emptyRules) {
    const Input input = input_from_string("abcdebcdeabc");
    {
        Rules rules { };
        std::vector<uint8_t> should_be {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 'b', 'c', 'd', 'e', 'b', 'c', 'd', 'e', 'a', 'b', 'c'
        };

        Code1Coder coder;
        auto coded = ostream_to_bytes([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
}

TEST(Code1Coder, emptyInput) {
    const Input input = input_from_string("");
    {
        Rules rules { };
        std::vector<uint8_t> should_be {
            0, 0, 0, 0, 0, 0, 0, 0,
            17,
        };

        Code1Coder coder;
        auto coded = ostream_to_bytes([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
}

TEST(Code2Coder, basic) {
    const Input input = input_from_string("abcdebcdeabc");
    {
        Rules rules { {1, 5, 4}, {5, 10, 2} };
        std::vector<uint8_t> should_be {
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
        };

        Code2Coder coder;
        auto coded = ostream_to_bytes([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
    {
        Rules rules { {5, 1, 4}, {9, 0, 3} };
        std::vector<uint8_t> should_be {
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
        };

        Code2Coder coder;
        auto coded = ostream_to_bytes([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });

        ASSERT_EQ(should_be, coded);
    }
}

TEST(Code2Coder, emptyRules) {
    const Input input = input_from_string("abcdebcdeabc");
    {
        Rules rules { };
        std::vector<uint8_t> should_be {
            0, 0, 0, 0, 0, 0, 0, 12,
            0, 0, 0, 2,
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

        };

        Code2Coder coder;
        auto coded = ostream_to_bytes([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
}

TEST(Code2Coder, emptyInput) {
    const Input input = input_from_string("");
    {
        Rules rules { };
        std::vector<uint8_t> should_be {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 2,
            1,
            1,
            1,
            0, 0,
            0, 0,
        };

        Code2Coder coder;
        auto coded = ostream_to_bytes([&] (std::ostream& out) {
            coder.code(rules, input, 2, out);
        });
        ASSERT_EQ(should_be, coded);
    }
}

TEST(Code0Decoder, basic) {
    const std::string should_be("abcdebcdeabc");

    {
        std::istringstream inp("12:a{6,4}{11,2}deabc");

        Code0Coder decoder;
        auto decoded = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(inp, out);
        });
        ASSERT_EQ(should_be, decoded);
    }
    {
        std::istringstream inp("12:abcde{2,4}{1,3}");

        Code0Coder decoder;
        auto decoded = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(inp, out);
        });
        ASSERT_EQ(should_be, decoded);
    }
    {
        std::istringstream inp("12:abcde{2,2}dea{6,2}");

        Code0Coder decoder;
        auto decoded = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(inp, out);
        });
        ASSERT_EQ(should_be, decoded);
    }
    {
        const std::string should_be("abcdefabcdefcd");
        std::istringstream inp("14:{7,3}{10,3}abcdef{3,2}");

        Code0Coder decoder;
        auto decoded = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(inp, out);
        });
        ASSERT_EQ(should_be, decoded);
    }
}

TEST(Code1Decoder, basic) {
    const std::string should_be("abcdebcdeabc");

    {
        std::vector<uint8_t> inp_bytes {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 1, 5, 4, 1, 10, 2, 'd', 'e', 'a', 'b', 'c'
        };

        std::string inp_str(inp_bytes.begin(), inp_bytes.end());
        std::istringstream inp(inp_str);
        Code1Coder decoder;
        auto decoded = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(inp, out);
        });
        ASSERT_EQ(should_be, decoded);
    }
    {
        std::vector<uint8_t> inp_bytes {
            0, 0, 0, 0, 0, 0, 0, 12,
            17,
            'a', 'b', 'c', 'd', 'e', 1, 1, 4, 1, 0, 3
        };

        std::string inp_str(inp_bytes.begin(), inp_bytes.end());
        std::istringstream inp(inp_str);
        Code1Coder decoder;
        auto decoded = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(inp, out);
        });
        ASSERT_EQ(should_be, decoded);
    }
}

TEST(Code2Decoder, basic) {
    const std::string should_be("abcdebcdeabc");

    {
        std::vector<uint8_t> inp_bytes {
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
        };

        std::string inp_str(inp_bytes.begin(), inp_bytes.end());
        std::istringstream inp(inp_str);
        Code2Coder decoder;
        auto decoded = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(inp, out);
        });
        ASSERT_EQ(should_be, decoded);
    }
    {
        std::vector<uint8_t> inp_bytes {
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
        };

        std::string inp_str(inp_bytes.begin(), inp_bytes.end());
        std::istringstream inp(inp_str);
        Code2Coder decoder;
        auto decoded = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(inp, out);
        });
        ASSERT_EQ(should_be, decoded);
    }
}

TEST(Roundtrip, ESACompressorMaxLCPSortedSuffixListCode0Coder) {
    test_roundtrip_batch<ESACompressor<MaxLCPSortedSuffixList>, Code0Coder>();
}

TEST(Roundtrip, ESACompressorMaxLCPSortedSuffixListCode1Coder) {
    test_roundtrip_batch<ESACompressor<MaxLCPSortedSuffixList>, Code1Coder>();
}

TEST(Roundtrip, ESACompressorMaxLCPSortedSuffixListCode2Coder) {
    test_roundtrip_batch<ESACompressor<MaxLCPSortedSuffixList>, Code2Coder>();
}

TEST(Roundtrip, ESACompressorMaxLCPHeapCode0Coder) {
    test_roundtrip_batch<ESACompressor<MaxLCPHeap>, Code0Coder>();
}

TEST(Roundtrip, ESACompressorMaxLCPHeapCode1Coder) {
    test_roundtrip_batch<ESACompressor<MaxLCPHeap>, Code1Coder>();
}

TEST(Roundtrip, ESACompressorMaxLCPHeapCode2Coder) {
    test_roundtrip_batch<ESACompressor<MaxLCPHeap>, Code2Coder>();
}
