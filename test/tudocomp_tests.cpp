#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <boost/filesystem.hpp>
#include <gtest/gtest.h>

#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/DecodeBuffer.hpp>

#include "test_util.h"

using namespace tudocomp;
using namespace tudocomp::io;

TEST(Test, test_file) {
    ASSERT_EQ(test_file_path("test.txt"), "test_files/test.txt");
    write_test_file("test.txt", "foobar");
    ASSERT_EQ(read_test_file("test.txt"), "foobar");

    std::string err;
    try {
        read_test_file("not_test.txt");
    } catch (std::runtime_error e) {
        err = e.what();
    }
    ASSERT_EQ("Could not open test file \"test_files/not_test.txt\"", err);
}

TEST(Util, bitsFor) {
    ASSERT_EQ(bitsFor(0b0), 1u);
    ASSERT_EQ(bitsFor(0b1), 1u);
    ASSERT_EQ(bitsFor(0b10), 2u);
    ASSERT_EQ(bitsFor(0b11), 2u);
    ASSERT_EQ(bitsFor(0b100), 3u);
    ASSERT_EQ(bitsFor(0b111), 3u);
    ASSERT_EQ(bitsFor(0b1000), 4u);
    ASSERT_EQ(bitsFor(0b1111), 4u);
    ASSERT_EQ(bitsFor(0b10000), 5u);
    ASSERT_EQ(bitsFor(0b11111), 5u);
    ASSERT_EQ(bitsFor(0b100000), 6u);
    ASSERT_EQ(bitsFor(0b111111), 6u);
    ASSERT_EQ(bitsFor(0b1000000), 7u);
    ASSERT_EQ(bitsFor(0b1111111), 7u);
    ASSERT_EQ(bitsFor(0b10000000), 8u);
    ASSERT_EQ(bitsFor(0b11111111), 8u);
    ASSERT_EQ(bitsFor(0b100000000), 9u);
    ASSERT_EQ(bitsFor(0b111111111), 9u);
}

TEST(Util, bytesFor) {
    ASSERT_EQ(bytesFor(0), 0u);
    ASSERT_EQ(bytesFor(1), 1u);
    ASSERT_EQ(bytesFor(8), 1u);
    ASSERT_EQ(bytesFor(9), 2u);
    ASSERT_EQ(bytesFor(16), 2u);
    ASSERT_EQ(bytesFor(17), 3u);
    ASSERT_EQ(bytesFor(24), 3u);
    ASSERT_EQ(bytesFor(25), 4u);
    ASSERT_EQ(bytesFor(32), 4u);
}

TEST(Util, pack_integers) {
    ASSERT_EQ(pack_integers({
        0b1111, 4,
        0b1001, 4,
    }), (std::vector<uint8_t> {
        0b11111001
    }));
    ASSERT_EQ(pack_integers({
        0b1111, 2,
    }), (std::vector<uint8_t> {
        0b11000000
    }));
    ASSERT_EQ(pack_integers({
        0b111, 3,
        0b1101, 4,
        0b11001, 5,
        0b110001, 6,
    }), (std::vector<uint8_t> {
        0b11111011,
        0b10011100,
        0b01000000,
    }));
    ASSERT_EQ(pack_integers({
        0b001000, 6,
    }), (std::vector<uint8_t> {
        0b00100000,
    }));
    ASSERT_EQ(pack_integers({
        0b001000, 6,
        0b001100001, 9,
        0b001100010, 9
    }), (std::vector<uint8_t> {
        0b00100000,
        0b11000010,
        0b01100010,
    }));
    ASSERT_EQ(pack_integers({
        9, 64,
    }), (std::vector<uint8_t> {
        0,0,0,0,0,0,0,9
    }));
}

TEST(Input, vector) {
    std::vector<uint8_t> v { 97, 98, 99 };
    Input inp = Input::from_memory(v);

    {
        auto guard = inp.as_view();
        auto ref = *guard;

        ASSERT_EQ(ref, "abc");
    }

    {
        auto guard = inp.as_stream();
        auto& stream = *guard;

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

TEST(Input, string_ref) {
    boost::string_ref v { "abc" };
    Input inp = Input::from_memory(v);

    {
        auto guard = inp.as_view();
        auto ref = *guard;

        ASSERT_EQ(ref, "abc");
    }

    {
        auto guard = inp.as_stream();
        auto& stream = *guard;

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

std::string fn(std::string suffix) {
    return "tudocomp_tests_" + suffix;
}

TEST(Input, file) {
    write_test_file(fn("short.txt"), "abc");
    Input inp = Input::from_path(test_file_path(fn("short.txt")));

    {
        auto guard = inp.as_view();
        auto ref = *guard;

        ASSERT_EQ(ref, "abc");
    }

    {
        auto guard = inp.as_stream();
        auto& stream = *guard;

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

TEST(Input, stream_view) {
    ViewStream stream { (char*) "abc", 3 };

    Input inp = Input::from_stream(stream.stream());

    {
        auto guard = inp.as_stream();
        auto& stream = *guard;

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

TEST(Input, stream_stream) {
    ViewStream stream { (char*) "abc", 3 };

    Input inp = Input::from_stream(stream.stream());

    {
        auto guard = inp.as_stream();
        auto& stream = *guard;

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

TEST(Output, memory) {
    std::vector<uint8_t> vec;

    Output out = Output::from_memory(vec);

    {
        auto guard = out.as_stream();
        auto& stream = *guard;

        stream << "abc";
    }

    ASSERT_EQ(vec, (std::vector<uint8_t> { 97, 98, 99 }));
}

TEST(Output, file) {
    Output out = Output::from_path(test_file_path(fn("short_out.txt")));

    {
        auto guard = out.as_stream();
        auto& stream = *guard;

        stream << "abc";
    }

    ASSERT_EQ(read_test_file(fn("short_out.txt")), "abc");
}

TEST(Output, stream) {
    std::stringstream ss;
    Output out = Output::from_stream(ss);

    {
        auto guard = out.as_stream();
        auto& stream = *guard;

        stream << "abc";
    }

    ASSERT_EQ(ss.str(), "abc");
}

TEST(IO, bits) {
    std::stringstream ss_out;
    BitOStream out(ss_out);
    
    out.writeBit(0);                   //0
    out.writeBit(1);                   //1
    out.write(-1, 2);                  //11
    out.write(0b11010110, 4);          //0110
    out.write_compressed_int(0x27, 3); //1 111 0 100
    out.write_compressed_int(0x33);    //0 0110011
    //output should contain 0111 0110 1111 0100 0011 0011 = 76 F4 33
    
    std::string result = ss_out.str();
    ASSERT_EQ(result.length(), 3U); //24 bits = 3 bytes
    
    bool done; //???
    
    //basic input test
    {
        std::stringstream ss_in(result);
        BitIStream in(ss_in, done);
        
        ASSERT_EQ(in.readBits<uint32_t>(24), 0x76F433U);
    }
    
    //advanced input test
    {
        std::stringstream ss_in(result);
        BitIStream in(ss_in, done);
        
        ASSERT_EQ(in.readBit(), 0);
        ASSERT_EQ(in.readBit(), 1);
        ASSERT_EQ(in.readBits<size_t>(2), 3U);
        ASSERT_EQ(in.readBits<size_t>(4), 6U);
        ASSERT_EQ(in.read_compressed_int<size_t>(3), 0x27U);
        ASSERT_EQ(in.read_compressed_int<size_t>(), 0x33U);
    }
}

TEST(DecodeBuffer, cbstrategy_none) {
    

    DecodeBuffer buffer(12, DCBStrategyNone());
    buffer.decode('b');
    buffer.decode('a');
    buffer.decode('n');
    buffer.defact(1, 3);
    buffer.defact(0, 6);
    
    std::stringstream ss;
    buffer.write_to(ss);
    
    ASSERT_EQ("bananabanana", ss.str());
    
}

TEST(DecodeBuffer, cbstrategy_map) {
    
    DecodeBuffer buffer(12, DCBStrategyMap());
    buffer.decode('b');
    buffer.defact(3, 3);
    buffer.decode('n');
    buffer.decode('a');
    buffer.defact(0, 6);
    
    std::stringstream ss;
    buffer.write_to(ss);
    
    ASSERT_EQ("bananabanana", ss.str());
    
}

TEST(DecodeBuffer, cbstrategy_array) {
    
    DecodeBuffer buffer(12, DCBStrategyRetargetArray(12));
    buffer.decode('b');
    buffer.defact(3, 3);
    buffer.decode('n');
    buffer.decode('a');
    buffer.defact(0, 6);
    
    std::stringstream ss;
    buffer.write_to(ss);
    
    ASSERT_EQ("bananabanana", ss.str());
    
}
