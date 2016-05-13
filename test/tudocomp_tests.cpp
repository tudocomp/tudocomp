#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/DecodeBuffer.hpp>
#include <tudocomp/util/View.hpp>

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

    {
        Input inp = Input::from_memory(v);
        auto guard = inp.as_view();
        auto ref = *guard;

        ASSERT_EQ(ref, "abc");
    }

    {
        Input inp = Input::from_memory(v);
        auto guard = inp.as_stream();
        auto& stream = *guard;

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

TEST(Input, string_ref) {
    string_ref v { "abc" };

    {
        Input inp = Input::from_memory(v);
        auto guard = inp.as_view();
        auto ref = *guard;

        ASSERT_EQ(ref, "abc");
    }

    {
        Input inp = Input::from_memory(v);
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

    {
        Input inp = Input::from_path(test_file_path(fn("short.txt")));
        auto guard = inp.as_view();
        auto ref = *guard;

        ASSERT_EQ(ref, "abc");
    }

    {
        Input inp = Input::from_path(test_file_path(fn("short.txt")));
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
    Output out = Output::from_path(test_file_path(fn("short_out.txt")), true);

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


    DecodeBuffer<DCBStrategyNone> buffer(12);
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

    DecodeBuffer<DCBStrategyMap> buffer(12);
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

    DecodeBuffer<DCBStrategyRetargetArray> buffer(12);
    buffer.decode('b');
    buffer.defact(3, 3);
    buffer.decode('n');
    buffer.decode('a');
    buffer.defact(0, 6);

    std::stringstream ss;
    buffer.write_to(ss);

    ASSERT_EQ("bananabanana", ss.str());

}

TEST(View, construction) {
    static const uint8_t DATA[3] = { 'f', 'o', 'o' };

    View a(DATA, sizeof(DATA));
    ASSERT_EQ(a.size(), 3u);
    ASSERT_EQ(a[0], 'f');
    ASSERT_EQ(a[2], 'o');

    View b(a);
    ASSERT_EQ(b.size(), 3u);
    ASSERT_EQ(b[0], 'f');
    ASSERT_EQ(b[2], 'o');

    View c(std::move(a));
    ASSERT_EQ(c.size(), 3u);
    ASSERT_EQ(c[0], 'f');
    ASSERT_EQ(c[2], 'o');

    std::vector<uint8_t> vec { 'b', 'a', 'r' };
    View d(vec);
    ASSERT_EQ(d.size(), 3u);
    ASSERT_EQ(d[0], 'b');
    ASSERT_EQ(d[2], 'r');

    View e("bar");
    ASSERT_EQ(e.size(), 3u);
    ASSERT_EQ(e[0], 'b');
    ASSERT_EQ(e[2], 'r');

    std::string str("bar");
    View f(str);
    ASSERT_EQ(f.size(), 3u);
    ASSERT_EQ(f[0], 'b');
    ASSERT_EQ(f[2], 'r');

    View g("qux!!", 3);
    ASSERT_EQ(g.size(), 3u);
    ASSERT_EQ(g[0], 'q');
    ASSERT_EQ(g[2], 'x');
}

TEST(View, conversion) {
    View s("xyz");

    std::string a(s);
    ASSERT_EQ(a, "xyz");

    std::vector<uint8_t> b(s);
    ASSERT_EQ(b, (std::vector<uint8_t> { 'x', 'y', 'z' }));
}

TEST(View, indexing) {
    const char* s = "abcde";

    View a(s);
    ASSERT_EQ(a.at(0), 'a');
    ASSERT_EQ(a.at(4), 'e');
    ASSERT_EQ(a[0], 'a');
    ASSERT_EQ(a[4], 'e');

    try {
        a.at(5);
    } catch (std::out_of_range& e) {
        ASSERT_EQ(std::string(e.what()),
                  "indexed view of size 5 with out-of-bounds value 5");
    }

#ifdef DEBUG
    try {
        a[5];
    } catch (std::out_of_range& e) {
        ASSERT_EQ(std::string(e.what()),
                  "indexed view of size 5 with out-of-bounds value 5");
    }
#endif

    ASSERT_EQ(a.front(), 'a');
    ASSERT_EQ(a.back(), 'e');
    ASSERT_EQ(a.data(), (const uint8_t*) s);
}

TEST(View, iterators) {
    View s("abc");

    std::vector<uint8_t> a (s.begin(), s.end());
    ASSERT_EQ(a, (std::vector<uint8_t> { 'a', 'b', 'c' }));

    std::vector<uint8_t> b (s.cbegin(), s.cend());
    ASSERT_EQ(b, (std::vector<uint8_t> { 'a', 'b', 'c' }));

    std::vector<uint8_t> c (s.rbegin(), s.rend());
    ASSERT_EQ(c, (std::vector<uint8_t> { 'c', 'b', 'a' }));

    std::vector<uint8_t> d (s.crbegin(), s.crend());
    ASSERT_EQ(d, (std::vector<uint8_t> { 'c', 'b', 'a' }));
}

TEST(View, capacity) {
    View s("abc");

    ASSERT_EQ(s.empty(), false);
    ASSERT_EQ(s.size(), 3u);
    ASSERT_EQ(s.max_size(), 3u);

    View t("");

    ASSERT_EQ(t.empty(), true);
    ASSERT_EQ(t.size(), 0u);
    ASSERT_EQ(t.max_size(), 0u);
}

TEST(View, modifiers) {
    View a("abc");
    ASSERT_EQ(a.size(), 3u);

    View b("");
    ASSERT_EQ(b.size(), 0u);

    a.swap(b);
    ASSERT_EQ(a.size(), 0u);
    ASSERT_EQ(b.size(), 3u);

    swap(a, b);
    ASSERT_EQ(a.size(), 3u);
    ASSERT_EQ(b.size(), 0u);

    View c("asdf");
    c.remove_prefix(1);
    ASSERT_EQ(c, "sdf");
    c.remove_suffix(1);
    ASSERT_EQ(c, "sd");
    c.clear();
    ASSERT_EQ(c, "");
}

TEST(View, comparision) {
    View a("abc");
    View b("acc");
    View c("");

    ASSERT_TRUE(a == a);
    ASSERT_TRUE(c == c);

    ASSERT_TRUE(a != b);
    ASSERT_TRUE(a != c);

    ASSERT_TRUE(a < b);
    ASSERT_TRUE(a <= b);

    ASSERT_TRUE(b > a);
    ASSERT_TRUE(b >= a);

    ASSERT_TRUE(c < a);
    ASSERT_TRUE(c < b);
}

TEST(View, ostream) {
    std::stringstream ss;
    ss << View("abc");
    ASSERT_EQ(ss.str(), std::string("abc"));
}

TEST(View, slicing) {
    View a("abc");
    ASSERT_EQ(a.size(), 3u);
    ASSERT_EQ(a, "abc");

    View b = a.substr(0, 3);
    ASSERT_EQ(b.size(), 3u);
    ASSERT_EQ(b, "abc");

    View c = a.substr(0);
    ASSERT_EQ(c.size(), 3u);
    ASSERT_EQ(c, "abc");

    View d = a.substr(2);
    ASSERT_EQ(d.size(), 1u);
    ASSERT_EQ(d, "c");

    View e = a.substr(2, 3);
    ASSERT_EQ(e.size(), 1u);
    ASSERT_EQ(e, "c");

    View f = a.substr(1, 2);
    ASSERT_EQ(f.size(), 1u);
    ASSERT_EQ(f, "b");
}


TEST(View, string_predicates) {
    View a("abc");
    View aa("abcabc");

    ASSERT_TRUE(a.starts_with('a'));
    ASSERT_TRUE(a.starts_with(uint8_t('a')));
    ASSERT_TRUE(a.ends_with('c'));
    ASSERT_TRUE(a.ends_with(uint8_t('c')));

    ASSERT_TRUE(a.starts_with(a));
    ASSERT_TRUE(a.ends_with(a));
    ASSERT_TRUE(aa.starts_with(a));
    ASSERT_TRUE(aa.ends_with(a));
}
