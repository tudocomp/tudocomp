#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include <tudocomp/tudocomp.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/DecodeBuffer.hpp>
#include <tudocomp/util/View.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Algorithm.hpp>

#include "test_util.h"
#include "tudocomp_test_util.h"

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

TEST(Test, test_file_null) {
    write_test_file("test_nte.txt", "foo\0bar\0"_v);
    ASSERT_EQ(read_test_file("test_nte.txt"), "foo\0bar\0"_v);
    ASSERT_NE(read_test_file("test_nte.txt"), "foo");
}

TEST(View, string_conv_null) {
    ASSERT_EQ("abc\0def\0"_v, std::string("abc\0def\0"_v));
    ASSERT_NE("abc\0dez\0"_v, std::string("abc\0def\0"_v));
    ASSERT_NE("abc\0def\0"_v, std::string("abc\0dez\0"_v));

    std::string x("x");
    x = x + std::string("abc\0def\0"_v);

    ASSERT_EQ(x, "xabc\0def\0"_v);
    ASSERT_NE(x, "xabc\0dez\0"_v);

    ASSERT_EQ(std::string("\xff\0"_v).size(), 2);
}

TEST(Util, bits_for) {
    ASSERT_EQ(bits_for(0b0), 1u);
    ASSERT_EQ(bits_for(0b1), 1u);
    ASSERT_EQ(bits_for(0b10), 2u);
    ASSERT_EQ(bits_for(0b11), 2u);
    ASSERT_EQ(bits_for(0b100), 3u);
    ASSERT_EQ(bits_for(0b111), 3u);
    ASSERT_EQ(bits_for(0b1000), 4u);
    ASSERT_EQ(bits_for(0b1111), 4u);
    ASSERT_EQ(bits_for(0b10000), 5u);
    ASSERT_EQ(bits_for(0b11111), 5u);
    ASSERT_EQ(bits_for(0b100000), 6u);
    ASSERT_EQ(bits_for(0b111111), 6u);
    ASSERT_EQ(bits_for(0b1000000), 7u);
    ASSERT_EQ(bits_for(0b1111111), 7u);
    ASSERT_EQ(bits_for(0b10000000), 8u);
    ASSERT_EQ(bits_for(0b11111111), 8u);
    ASSERT_EQ(bits_for(0b100000000), 9u);
    ASSERT_EQ(bits_for(0b111111111), 9u);
}

TEST(Util, bytes_for) {
    ASSERT_EQ(bytes_for(0), 0u);
    ASSERT_EQ(bytes_for(1), 1u);
    ASSERT_EQ(bytes_for(8), 1u);
    ASSERT_EQ(bytes_for(9), 2u);
    ASSERT_EQ(bytes_for(16), 2u);
    ASSERT_EQ(bytes_for(17), 3u);
    ASSERT_EQ(bytes_for(24), 3u);
    ASSERT_EQ(bytes_for(25), 4u);
    ASSERT_EQ(bytes_for(32), 4u);
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
        auto ref = inp.as_view();

        ASSERT_EQ(ref, "abc");
    }

    {
        Input inp = Input::from_memory(v);
        auto stream = inp.as_stream();

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

TEST(Input, string_ref) {
    string_ref v { "abc" };

    {
        Input inp = Input::from_memory(v);
        auto ref = inp.as_view();

        ASSERT_EQ(ref, "abc");
    }

    {
        Input inp = Input::from_memory(v);
        auto stream = inp.as_stream();

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
        auto ref = inp.as_view();

        ASSERT_EQ(ref, "abc");
    }

    {
        Input inp = Input::from_path(test_file_path(fn("short.txt")));
        auto stream = inp.as_stream();

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

void stream_moving(Input& i) {
    {
        InputStream s1 = i.as_stream();
        ASSERT_EQ(s1.get(), 'a');
        InputStream s2(std::move(s1));
        ASSERT_EQ(s2.get(), 'b');
        InputStream s3(std::move(s2));
        ASSERT_EQ(s3.get(), 'c');
    }
    {
        InputStream s4 = i.as_stream();
        ASSERT_EQ(s4.get(), 'd');
        InputStream s5(std::move(s4));
        ASSERT_EQ(s5.get(), 'e');
    }
}

TEST(Input, stream_moving_vec) {
    std::vector<uint8_t> v { 97, 98, 99, 100, 101 };
    Input i(v);
    stream_moving(i);
}

TEST(Input, stream_moving_mem) {
    string_ref v { "abcde" };
    Input i(v);
    stream_moving(i);
}

TEST(Input, stream_moving_file) {
    write_test_file(fn("stream_moving.txt"), "abcde");
    Input i = Input::from_path(test_file_path(fn("stream_moving.txt")));
    stream_moving(i);
}

TEST(Input, stream_iterator) {
    Input i("asdf");

    std::stringstream ss;

    for (uint8_t x : i.as_stream()) {
        ss << x;
    }

    ASSERT_EQ(ss.str(), "asdf");
}

TEST(Input, ensure_null_term) {
    std::vector<uint8_t> a  { 96, 97, 98, 99, 100, 101, 102 };
    std::vector<uint8_t> a2 { 96, 97, 98, 99, 100, 101, 102 };
    std::vector<uint8_t> b  { 96, 97, 98 };
    std::vector<uint8_t> c  { 96, 97, 98, 0 };

    {
        Input i(View(a).substr(0, 3));
        auto x = i.as_view();
        std::vector<uint8_t> y = x;
        ASSERT_EQ(y, b);
        ASSERT_NE(y, c);
    }
    ASSERT_EQ(a, a2);

    {
        Input i(View(a).substr(0, 3));
        auto x = i.as_view();
        x.ensure_null_terminator();
        ASSERT_NE(x, b);
        ASSERT_EQ(x, c);
    }
    ASSERT_EQ(a, a2);

    {
        Input i(View(a).substr(0, 3));
        Input i2 = std::move(i);
        auto x = i2.as_view();
        x.ensure_null_terminator();
        ASSERT_NE(x, b);
        ASSERT_EQ(x, c);
    }
    ASSERT_EQ(a, a2);

    {
        Input i(View(a).substr(0, 3));

        Input i2 = i;

        auto x = i2.as_view();
        x.ensure_null_terminator();
        ASSERT_NE(x, b);
        ASSERT_EQ(x, c);

        auto y = i.as_view();
        y.ensure_null_terminator();
        ASSERT_NE(y, b);
        ASSERT_EQ(y, c);
    }
    ASSERT_EQ(a, a2);

    {
        Input i(View(a).substr(0, 3));
        {
            auto x = i.as_view();
            x.ensure_null_terminator();
            ASSERT_NE(x, b);
            ASSERT_EQ(x, c);
        }
        {
            auto x = i.as_view();
            ASSERT_EQ(x, ""_v);
        }
        {
            auto x = i.as_view();
            x.ensure_null_terminator();
            ASSERT_EQ(x, "\0"_v);
        }
        {
            auto x = i.as_view();
            ASSERT_EQ(x, ""_v);
        }
        {
            auto x = i.as_view();
            x.ensure_null_terminator();
            ASSERT_EQ(x, "\0"_v);
        }
    }
    ASSERT_EQ(a, a2);
}

namespace input_nte_matrix {
    using Path = Input::Path;

    std::vector<uint8_t>       slice_buf       { 97,  98,  99, 100, 101, 102 };
    const View                 slice           = View(slice_buf).substr(0, 3);
    const std::vector<uint8_t> o_slice         { 97,  98,  99  };

    template<class T, class CStrat>
    void matrix_row(T input,
                    View expected_output,
                    CStrat i_copy_strat,
                    std::function<void(Input&, View)> i_out_compare) {

        i_copy_strat(std::move(input),
                     expected_output,
                     [](Input& i) {},
                     i_out_compare);
    }

    View view(View i) {
        return i;
    }

    Input::Path file(View i) {
        std::hash<std::string> hasher;

        std::string basename = std::string("matrix_test_file_path") + std::string(i);
        //std::cout << "basename before: " << basename;
        std::stringstream ss;
        ss << hasher(basename);
        basename = ss.str() + ".txt";
        //std::cout << " basename after: " << basename << "\n";

        write_test_file(basename, i);
        return Input::Path { test_file_path(basename) };
    }

    template<class T>
    void no_copy(T i,
                 View o,
                 std::function<void(Input&)> i_nte_mod,
                 std::function<void(Input&, View)> i_out_compare) {
        Input target_input(std::move(i));
        i_nte_mod(target_input);

        i_out_compare(target_input, o);
        i_out_compare(target_input, "");

    }

    template<class T>
    void copy(T i,
              View o,
              std::function<void(Input&)> i_nte_mod,
              std::function<void(Input&, View)> i_out_compare) {
        Input target_input;
        {
            Input source_input(std::move(i));
            i_nte_mod(source_input);
            target_input = source_input;
            i_out_compare(source_input, o);
            i_out_compare(source_input, "");
        }

        i_out_compare(target_input, o);
        i_out_compare(target_input, "");
    }

    template<class T>
    void move(T i,
              View o,
              std::function<void(Input&)> i_nte_mod,
              std::function<void(Input&, View)> i_out_compare) {
        Input target_input;
        {
            Input source_input(std::move(i));
            i_nte_mod(source_input);
            target_input = std::move(source_input);
        }

        i_out_compare(target_input, o);
        i_out_compare(target_input, "");
    }

    void out_view(Input& i_, View o) {
        std::vector<uint8_t> b = o;
        std::vector<uint8_t> c = o;
        if (c.size() == 0 || c.back() != 0) {
            c.push_back(0);
        }

        // First, do a bunch of tests on the copies to check for the null terminator
        // behaving correctly.

        Input i_bak = i_;

        {
            Input i = i_bak;
            auto x = i.as_view();
            std::vector<uint8_t> y = x;
            ASSERT_EQ(y, b);
            ASSERT_NE(y, c);
        }
        {
            Input i = i_bak;
            auto x = i.as_view();
            x.ensure_null_terminator();
            ASSERT_NE(x, b);
            ASSERT_EQ(x, c);
        }

        {
            Input i = i_bak;
            Input i2 = i;

            auto x = i2.as_view();
            x.ensure_null_terminator();
            ASSERT_NE(x, b);
            ASSERT_EQ(x, c);

            auto y = i.as_view();
            y.ensure_null_terminator();
            ASSERT_NE(y, b);
            ASSERT_EQ(y, c);
        }

        {
            Input i = i_bak;
            {
                auto x = i.as_view();
                x.ensure_null_terminator();
                ASSERT_NE(x, b);
                ASSERT_EQ(x, c);
            }
            {
                auto x = i.as_view();
                ASSERT_EQ(x, ""_v);
            }
            {
                auto x = i.as_view();
                x.ensure_null_terminator();
                ASSERT_EQ(x, "\0"_v);
            }
            {
                auto x = i.as_view();
                ASSERT_EQ(x, ""_v);
            }
            {
                auto x = i.as_view();
                x.ensure_null_terminator();
                ASSERT_EQ(x, "\0"_v);
            }
        }

        // Then ensure we actually skip i forward in the end:
        i_.as_view();
    }

    void out_stream(Input& i, View o) {
        auto tmp = i.as_stream();
        std::stringstream ss;
        ss << tmp.rdbuf();
        std::string a_ = ss.str();
        std::vector<uint8_t> a = View(a_);
        std::vector<uint8_t> b = o;
        ASSERT_EQ(a, b);
    }

    TEST(InputNteMatrix, n1) {
        matrix_row(view(slice), View(o_slice), no_copy<View>, out_view);
    }

    TEST(InputNteMatrix, n2) {
        matrix_row(view(slice), View(o_slice), no_copy<View>, out_stream);
    }

    TEST(InputNteMatrix, n7) {
        matrix_row(view(slice), View(o_slice), copy<View>, out_view);
    }

    TEST(InputNteMatrix, n8) {
        matrix_row(view(slice), View(o_slice), copy<View>, out_stream);
    }

    TEST(InputNteMatrix, n13) {
        matrix_row(view(slice), View(o_slice), move<View>, out_view);
    }

    TEST(InputNteMatrix, n14) {
        matrix_row(view(slice), View(o_slice), move<View>, out_stream);
    }

    TEST(InputNteMatrix, n19) {
        matrix_row(file(slice), View(o_slice), no_copy<Path>, out_view);
    }

    TEST(InputNteMatrix, n20) {
        matrix_row(file(slice), View(o_slice), no_copy<Path>, out_stream);
    }

    TEST(InputNteMatrix, n25) {
        matrix_row(file(slice), View(o_slice), copy<Path>, out_view);
    }

    TEST(InputNteMatrix, n26) {
        matrix_row(file(slice), View(o_slice), copy<Path>, out_stream);
    }

    TEST(InputNteMatrix, n31) {
        matrix_row(file(slice), View(o_slice), move<Path>, out_view);
    }

    TEST(InputNteMatrix, n32) {
        matrix_row(file(slice), View(o_slice), move<Path>, out_stream);
    }

}

TEST(Output, memory) {
    std::vector<uint8_t> vec;

    Output out = Output::from_memory(vec);

    {
        auto stream = out.as_stream();

        stream << "abc";
    }

    ASSERT_EQ(vec, (std::vector<uint8_t> { 97, 98, 99 }));
}

TEST(Output, file) {
    Output out = Output::from_path(test_file_path(fn("short_out.txt")), true);

    {
        auto stream = out.as_stream();

        stream << "abc";
    }

    ASSERT_EQ(read_test_file(fn("short_out.txt")), "abc");
}

TEST(Output, stream) {
    std::stringstream ss;
    Output out = Output::from_stream(ss);

    {
        auto stream = out.as_stream();

        stream << "abc";
    }

    ASSERT_EQ(ss.str(), "abc");
}

TEST(IO, bits) {
    std::stringstream ss_out;
    BitOStream out(ss_out);

    out.write_bit(0);                   //0
    out.write_bit(1);                   //1
    out.write_int(-1, 2);               //11
    out.write_int(0b11010110, 4);       //0110
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

        ASSERT_EQ(in.read_int<uint32_t>(24), 0x76F433U);
    }

    //advanced input test
    {
        std::stringstream ss_in(result);
        BitIStream in(ss_in, done);

        ASSERT_EQ(in.read_bit(), 0);
        ASSERT_EQ(in.read_bit(), 1);
        ASSERT_EQ(in.read_int<size_t>(2), 3U);
        ASSERT_EQ(in.read_int<size_t>(4), 6U);
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
                  "accessing view with bounds [0, 5) at out-of-bounds index 5");
    }

#ifdef DEBUG
    try {
        a[5];
    } catch (std::out_of_range& e) {
        ASSERT_EQ(std::string(e.what()),
                  "accessing view with bounds [0, 5) at out-of-bounds index 5");
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

TEST(View, literal) {
    ASSERT_EQ("abc"_v, View("abc"));
    ASSERT_NE("abc"_v, "abc\0def\0"_v);
    ASSERT_EQ("abc\0def\0"_v, "abc\0def\0"_v);
    ASSERT_NE("abc\0def\0"_v, "abc"_v);

    ASSERT_EQ("a\0"_v.back(), 0);
}

struct MySubAlgo: Algorithm {
    MySubAlgo(Env&& e): Algorithm(std::move(e)) {}

    inline static Meta meta() {
        Meta y("sub_t", "sub1");
        y.option("x").dynamic("x");
        return y;
    }
};

struct MySubAlgo2: Algorithm {
    MySubAlgo2(Env&& e): Algorithm(std::move(e)) {}

    inline static Meta meta() {
        Meta y("sub_t", "sub2");
        y.option("y").dynamic("y");
        return y;
    }
};

template<class A>
struct MyCompressor: public Compressor {
    inline static Meta meta() {
        Meta y("compressor", "my");
        y.option("sub").templated<A, MySubAlgo2>();
        y.option("dyn").dynamic("foobar");
        y.option("bool_val").dynamic("true");
        return y;
    }

    std::string custom_data;
    MyCompressor() = delete;
    MyCompressor(Env&& env, std::string&& s):
        Compressor(std::move(env)),
        custom_data(std::move(s)) {}

    inline virtual void decompress(Input& input, Output& output) {}

    inline virtual void compress(Input& input, Output& output) {
        A a(env().env_for_option("sub"));
        auto s = output.as_stream();
        s << "ok! " << custom_data << " " << env().option("dyn").as_string();
        ASSERT_TRUE(env().option("bool_val").as_bool());
    }
};

TEST(Algorithm, create) {


    auto x = create_algo<MyCompressor<MySubAlgo>>("", "test");

    std::vector<uint8_t> vec;
    Output out(vec);
    Input inp("");
    x.compress(inp, out);

    auto s = vec_as_lossy_string(vec);

    ASSERT_EQ(s, "ok! test foobar");


}

// std::cout << __FILE__ ":" << __LINE__ << "\n";

TEST(Algorithm, meta) {
    using Compressor = MyCompressor<MySubAlgo>;
    using Compressor2 = MyCompressor<MySubAlgo2>;
    {
        auto x = Compressor::meta();
        auto y = std::move(x).build_def();
        ASSERT_EQ(y.to_string(),
                  R"(my(sub: static sub_t = sub2(y: string = "y"), dyn: string = "foobar", bool_val: string = "true"))");
    }
    {
        auto x = Compressor::meta();
        auto y = std::move(x).build_ast_value();
        ASSERT_EQ(y.to_string(),
                  R"(my(sub: static sub_t = sub2(y: string = "y"), dyn: string = "foobar", bool_val: string = "true"))");
    }
    auto f = [](const std::string& options, std::function<void(OptionValue&)> g) {
        auto x = Compressor::meta();
        auto y = std::move(x).build_static_args_ast_value();;

        // TODO: Test eval

        eval::AlgorithmTypes types;
        gather_types(types, {
            Compressor::meta(),
            Compressor2::meta()
        });

        // error case: no "" for dyn
        ast::Parser p { options };

        auto evald = eval::cl_eval(
            p.parse_value(),
            "compressor",
            types,
            std::move(y)
        );

        g(evald);
    };
    f("my()", [](OptionValue& options){
        auto& my = options.as_algorithm();
        ASSERT_EQ(my.name(), "my");
        {
            auto& sub = my.arguments().at("sub").as_algorithm();
            ASSERT_EQ(sub.name(), "sub1");
            {
                auto& x = sub.arguments().at("x").as_string();
                ASSERT_EQ(x, "x");
            }

            auto& dyn = my.arguments().at("dyn").as_string();
            ASSERT_EQ(dyn, "foobar");
        }
    });
    f("my(dyn = \"quxqux\")", [](OptionValue& options){
        auto& my = options.as_algorithm();
        ASSERT_EQ(my.name(), "my");
        {
            auto& sub = my.arguments().at("sub").as_algorithm();
            ASSERT_EQ(sub.name(), "sub1");
            {
                auto& x = sub.arguments().at("x").as_string();
                ASSERT_EQ(x, "x");
            }

            auto& dyn = my.arguments().at("dyn").as_string();
            ASSERT_EQ(dyn, "quxqux");
        }
    });
    f("my(sub = sub1, dyn = \"quxqux\")", [](OptionValue& options){
        auto& my = options.as_algorithm();
        ASSERT_EQ(my.name(), "my");
        {
            auto& sub = my.arguments().at("sub").as_algorithm();
            ASSERT_EQ(sub.name(), "sub1");
            {
                auto& x = sub.arguments().at("x").as_string();
                ASSERT_EQ(x, "x");
            }

            auto& dyn = my.arguments().at("dyn").as_string();
            ASSERT_EQ(dyn, "quxqux");
        }
    });
    f("my(sub = sub1(x = \"asdf\"), dyn = \"quxqux\")", [](OptionValue& options){
        auto& my = options.as_algorithm();
        ASSERT_EQ(my.name(), "my");
        {
            auto& sub = my.arguments().at("sub").as_algorithm();
            ASSERT_EQ(sub.name(), "sub1");
            {
                auto& x = sub.arguments().at("x").as_string();
                ASSERT_EQ(x, "asdf");
            }

            auto& dyn = my.arguments().at("dyn").as_string();
            ASSERT_EQ(dyn, "quxqux");
        }
    });
}
