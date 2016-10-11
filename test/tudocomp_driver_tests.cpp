#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <tudocomp/util.h>

#include "test_util.h"
#include <tudocomp_driver/Registry.hpp>
#include <tudocomp/AlgorithmStringParser.hpp>

#include "tudocomp_driver_util.h"

TEST(TudocompDriver, list) {
    // Test that we got at some output with --list

    auto list = driver("--list");

    std::cout << list;

    // Check that the output is long enough to not be empty
    ASSERT_GE(list.size(), 100u);

}

TEST(TudocompDriver, algorithm_header) {
    std::string text = "asdfghjklöä";
    bool abort = false;
    // Without header
    roundtrip("lz78(debug)", "_header_test_0", text, true, abort);

    // With header
    roundtrip("lz78(debug)", "_header_test_1", text, false, abort);

    ASSERT_FALSE(abort);

    std::string text0 = read_test_file(roundtrip_comp_file_name(
        "lz78(debug)", "_header_test_0"));

    ASSERT_TRUE(text0.find('(') == 0);

    std::string text1 = read_test_file(roundtrip_comp_file_name(
        "lz78(debug)", "_header_test_1"));

    ASSERT_TRUE(text1.find("lz78(debug)%(") == 0);

}

TEST(Registry, smoketest) {
    using namespace tdc_algorithms;
    using ast::Value;
    using ast::Arg;
    ast::Parser p { "foo(abc, def=ghi, x : static y = z, "
                    "jkl=mno(p, q=\"1\"), q:r)" };

    Value a = p.parse_value();

    ASSERT_TRUE(a.is_invokation());
    ASSERT_EQ(a.invokation_name(), "foo");
    auto& b = a.invokation_arguments();
    ASSERT_EQ(b.size(), 5);

        ASSERT_FALSE(b[0].has_keyword());
        ASSERT_FALSE(b[0].has_type());
        ASSERT_EQ(b[0].value().invokation_name(), "abc");

        ASSERT_TRUE(b[1].has_keyword());
        ASSERT_FALSE(b[1].has_type());
        ASSERT_EQ(b[1].keyword(), "def");
        ASSERT_EQ(b[1].value().invokation_name(), "ghi");

        ASSERT_TRUE(b[2].has_keyword());
        ASSERT_TRUE(b[2].has_type());
        ASSERT_TRUE(b[2].type_is_static());
        ASSERT_EQ(b[2].keyword(), "x");
        ASSERT_EQ(b[2].type(), "y");
        ASSERT_EQ(b[2].value().invokation_name(), "z");

        ASSERT_TRUE(b[3].has_keyword());
        ASSERT_FALSE(b[3].has_type());
        ASSERT_FALSE(b[3].type_is_static());
        ASSERT_EQ(b[3].keyword(), "jkl");
        ASSERT_EQ(b[3].value().invokation_name(), "mno");

        auto& c = b[3].value().invokation_arguments();
        ASSERT_EQ(c.size(), 2);

            ASSERT_FALSE(c[1].value().is_invokation());
            ASSERT_EQ(c[1].value().string_value(), "1");

        ASSERT_FALSE(b[4].has_keyword());
        ASSERT_TRUE(b[4].has_type());
        ASSERT_EQ(b[4].value().invokation_name(), "q");
        ASSERT_EQ(b[4].type(), "r");
}

TEST(Registry, decl) {
    using namespace tdc_algorithms;
    ast::Parser p {
        "foo(a: b, c: d = e, f: static g)"
    };

    ast::Value a = p.parse_value();

    decl::Algorithm b = decl::from_ast(std::move(a), "blub");

    ASSERT_EQ(b.name(), "foo");
    ASSERT_EQ(b.arguments().size(), 3);

    ASSERT_EQ(b.arguments()[0].name(), "a");
    ASSERT_EQ(b.arguments()[0].type(), "b");
    ASSERT_FALSE(b.arguments()[0].is_static());

    ASSERT_EQ(b.arguments()[1].name(), "c");
    ASSERT_EQ(b.arguments()[1].type(), "d");
    ASSERT_TRUE(b.arguments()[1].has_default());
    ASSERT_EQ(b.arguments()[1].default_value().invokation_name(), "e");
    ASSERT_FALSE(b.arguments()[1].is_static());

    ASSERT_EQ(b.arguments()[2].name(), "f");
    ASSERT_EQ(b.arguments()[2].type(), "g");
    ASSERT_TRUE(b.arguments()[2].is_static());

}

TEST(Registry, lookup) {
    using namespace tdc_algorithms;
    Registry& r = REGISTRY;
    auto c = r.select_algorithm_or_exit("lz78(dict_size = \"100\")");
}

TEST(Registry, dynamic_options) {
    using namespace tdc_algorithms;

    Registry& r = REGISTRY;

    struct MySub {
        inline static Meta meta() {
            Meta y("b", "x");
            y.option("l").dynamic("zzz");
            return y;
        }
    };
    struct MyCompressor: public Compressor {
        inline static Meta meta() {
            Meta y("compressor", "foo");
            y.option("a").templated<MySub>();
            y.option("c").dynamic();
            y.option("d").dynamic("asdf");
            return y;
        }

        using Compressor::Compressor;
        inline virtual void decompress(Input& input, Output& output) {}

        inline virtual void compress(Input& input, Output& output) {
            auto s = output.as_stream();
            auto t = input.as_view();

            ASSERT_EQ(t, "test");
            s << "check";

            ASSERT_TRUE((env().option("a"), true));
            ASSERT_TRUE((env().option("c"), true));
            ASSERT_TRUE((env().option("d"), true));

            ASSERT_EQ(env().option("c").as_string(), "qwerty");
            ASSERT_EQ(env().option("d").as_string(), "asdf");

            auto& a = env().option("a").as_algorithm();
            auto& a_options = a.arguments();
            ASSERT_EQ(a.name(), "x");
            ASSERT_EQ(a_options.at("l").as_string(), "zzz");
        }
    };

    r.register_compressor<MyCompressor>();

    auto c = r.select_algorithm_or_exit("foo(x, \"qwerty\")");
    std::vector<uint8_t> data;
    Output out(data);
    Input inp("test");
    c->compress(inp, out);

    ASSERT_EQ(View(data), "check");
}

TEST(TudocompDriver, all_compressors_defined) {
    using namespace tdc_algorithms;

    Registry& r = REGISTRY;
    auto s = r.check_for_undefined_compressors();
    bool has_undefined_compressors = s.size() > 0;
    if (has_undefined_compressors) {
        std::stringstream ss;
        for (auto& s2 : s) {
            ss << "Undefined compressor: " << s2 << "\n";
        }
        EXPECT_FALSE(has_undefined_compressors) << ss.str();
    }
}
