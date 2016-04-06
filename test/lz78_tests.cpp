#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>

#include <gtest/gtest.h>

#include "test_util.h"
#include "tudocomp_test_util.h"

#include <tudocomp/lz78/Factor.hpp>
#include <tudocomp/lz78/Lz78DebugCoder.hpp>
#include <tudocomp/lz78/Lz78BitCoder.hpp>
#include <tudocomp/lz78/Lz78Compressor.hpp>
#include <tudocomp/lz78/lzcics/Lz78cicsCompressor.hpp>

using tudocomp::lz78::Factor;

using tudocomp::Input;
using tudocomp::Output;
using tudocomp::Env;

using std::swap;

TEST(Factor, ostream1) {
    std::stringstream s;
    Factor test { 0, 'x' };
    s << test;
    ASSERT_EQ(s.str(), "(0, 'x')");
}

TEST(Factor, ostream2) {
    std::stringstream s;
    Factor test { 0, '\0' };
    s << test;
    ASSERT_EQ(s.str(), "(0, 0)");
}

TEST(Factor, ostream3) {
    std::stringstream s;
    Factor test { 0, 255 };
    s << test;
    ASSERT_EQ(s.str(), "(0, 255)");
}

TEST(Lz78, Lz78DebugCoder) {
    using Coder = tudocomp::lz78::Lz78DebugCoder;

    Env env;
    std::vector<uint8_t> encoded_buffer;
    Output encoded_out = Output::from_memory(encoded_buffer);

    {
        Coder coder(env, encoded_out);

        coder.encode_fact(Factor {0, 'x'});
        coder.encode_fact(Factor {0, 'y'});
        coder.encode_fact(Factor {1, 'a'});
        coder.encode_fact(Factor {2, 'b'});
        coder.encode_fact(Factor {3, '!'});
    }

    auto encoded = std::string(encoded_buffer.begin(), encoded_buffer.end());
    ASSERT_EQ(encoded, "(0,x)(0,y)(1,a)(2,b)(3,!)");

    Input input(encoded);
    std::vector<uint8_t> decoded_buffer;
    auto decoded_out = Output::from_memory(decoded_buffer);

    Coder::decode(input, decoded_out);
    std::string decoded(decoded_buffer.begin(), decoded_buffer.end());

    ASSERT_EQ(decoded, "xyxaybxa!");

}

TEST(Lz78, Lz78BitCoder) {
    using Coder = tudocomp::lz78::Lz78BitCoder;

    Env env;
    std::vector<uint8_t> encoded_buffer;
    Output encoded_out = Output::from_memory(encoded_buffer);

    {
        Coder coder(env, encoded_out);

        coder.encode_fact(Factor {0, 'x'});
        coder.encode_fact(Factor {0, 'y'});
        coder.encode_fact(Factor {1, 'a'});
        coder.encode_fact(Factor {2, 'b'});
        coder.encode_fact(Factor {3, '!'});
        coder.encode_fact(Factor {5, '?'});
    }

    auto encoded = pack_integers({
        0, 1, 'x', 8,
        0, 1, 'y', 8,
        1, 2, 'a', 8,
        2, 2, 'b', 8,
        3, 3, '!', 8,
        5, 3, '?', 8,
    });
    ASSERT_EQ(encoded_buffer, encoded);

    auto input = Input::from_memory(encoded);
    std::vector<uint8_t> decoded_buffer;
    auto decoded_out = Output::from_memory(decoded_buffer);

    Coder::decode(input, decoded_out);
    std::string decoded(decoded_buffer.begin(), decoded_buffer.end());

    ASSERT_EQ(decoded, "xyxaybxa!xa!?");

}

TEST(Lz78, Lz78Compressor) {
    using Coder = tudocomp::lz78::Lz78DebugCoder;
    using Compressor = tudocomp::lz78::Lz78Compressor<Coder>;

    Env env;
    boost::string_ref input_str = "xyxaybxa!xa!?";

    std::vector<uint8_t> encoded_buffer;
    std::vector<uint8_t> decoded_buffer;
    Compressor compressor(env);

    {
        auto input = Input::from_memory(input_str);
        auto out = Output::from_memory(encoded_buffer);
        compressor.compress(input, out);
    }

    auto encoded = std::string(encoded_buffer.begin(), encoded_buffer.end());
    ASSERT_EQ(encoded, "(0,x)(0,y)(1,a)(2,b)(3,!)(5,?)");

    {
        auto input = Input::from_memory(encoded_buffer);
        auto out = Output::from_memory(decoded_buffer);
        compressor.decompress(input, out);
    }

    ASSERT_EQ(decoded_buffer, std::vector<uint8_t>(input_str.begin(), input_str.end()));

}

TEST(Lz78, compress) {
    using Coder = tudocomp::lz78::Lz78DebugCoder;
    using Compressor = tudocomp::lz78::Lz78Compressor<Coder>;

    {
        auto encoded = test::compress<Compressor>("abaaabab");
        ASSERT_EQ(encoded.str, "(0,a)(0,b)(1,a)(1,b)(1,b)");
        encoded.assert_decompress();
    }
    {
        auto encoded = test::compress<Compressor>("abcdebcdeabc");
        ASSERT_EQ(encoded.str, "(0,a)(0,b)(0,c)(0,d)(0,e)(2,c)(4,e)(1,b)(0,c)");
        encoded.assert_decompress();
    }
    {
        auto encoded = test::compress<Compressor>("ananas");
        ASSERT_EQ(encoded.str, "(0,a)(0,n)(1,n)(1,s)");
        encoded.assert_decompress();
    }
}
