#include <iostream>
#include <sstream>
#include <utility>
#include <algorithm>

#include "gtest/gtest.h"
#include "tudocomp.h"

using namespace tudocomp;

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

TEST(InputHandle, vector) {
    using namespace input;
    using Inp = input::Input;

    std::vector<uint8_t> v { 97, 98, 99 };
    Inp inp = Inp::from_memory(v);

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

TEST(InputHandle, file) {
    using namespace input;
    using Inp = input::Input;

    {
        // create a test file

        std::ofstream myfile;
        myfile.open ("short.txt");
        myfile << "abc";
        myfile.close();
    }

    Inp inp = Inp::from_path("short.txt");

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

TEST(InputHandle, stream_view) {
    using namespace input;
    using Inp = input::Input;

    ViewStream stream { (char*) "abc", 3 };

    Inp inp = Inp::from_stream(stream.stream());

    {
        auto guard = inp.as_stream();
        auto& stream = *guard;

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}

TEST(InputHandle, stream_stream) {
    using namespace input;
    using Inp = input::Input;

    ViewStream stream { (char*) "abc", 3 };

    Inp inp = Inp::from_stream(stream.stream());

    {
        auto guard = inp.as_stream();
        auto& stream = *guard;

        std::string s;

        stream >> s;

        ASSERT_EQ(s, "abc");
    }
}
