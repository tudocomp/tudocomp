#include <gtest/gtest.h>

#include "test/util.hpp"
#include <tudocomp/Literal.hpp>
#include <tudocomp/AlgorithmStringParser.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/coders/ArithmeticCoder.hpp>

using namespace tdc;

void test_arithm(const std::string& text) {
    //encode
    std::stringstream input(text);
    std::stringstream output;

    {//write
        tdc::io::Output out(output);
        ArithmeticCoder::Encoder encoder(create_env(ArithmeticCoder::meta()), out, ViewLiterals(text));

        {//now writing
            char c;
            while(input.get(c)) {
                encoder.encode(uliteral_t(c), tdc::literal_r);
            }
        }
    }

    {//read
        tdc::io::Input in(output);
        ArithmeticCoder::Decoder decoder(create_env(ArithmeticCoder::meta()), in);
        input.clear();
        input.str(std::string{});

        while(!decoder.eof()) {
            input << decoder.template decode<uliteral_t> (literal_r);
        }
    }
    ASSERT_EQ(input.str(), text);
    ASSERT_EQ(input.str().size(), text.size());
}

TEST(arithm, nullbyte) {
    test_arithm("hel\0lo");
    test_arithm("hello\0");
    test_arithm("hello\0\0\0");
}

TEST(arithm, stringgenerators) {
    std::function<void(std::string&)> func(test_arithm);
    test::on_string_generators(func,20);
}
