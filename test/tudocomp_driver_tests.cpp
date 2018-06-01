#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <tudocomp/util.hpp>

#include <tudocomp/AlgorithmStringParser.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp_driver/Registry.hpp>

#include "test/util.hpp"
#include "test/driver_util.hpp"

TEST(TudocompDriver, list) {
    // Test that we got at some output with --list

    auto list = driver_test::driver("--list");

    std::cout << list;

    // Check that the output is long enough to not be empty
    ASSERT_GE(list.size(), 100u);

}

TEST(TudocompDriver, algorithm_header) {
    std::string text = "asdfghhhhhhhhhhhhjklöä";
    bool abort = false;
    // Without header
    driver_test::roundtrip("rle", "_header_test_0", text, true, abort, true).check();

    // With header
    driver_test::roundtrip("rle", "_header_test_1", text, false, abort, true).check();

    ASSERT_FALSE(abort);

    std::string text0 = test::read_test_file(driver_test::roundtrip_comp_file_name(
        "rle", "_header_test_0"));

    ASSERT_FALSE(text0.find("rle%") == 0);

    std::string text1 = test::read_test_file(driver_test::roundtrip_comp_file_name(
        "rle", "_header_test_1"));

    ASSERT_TRUE(text1.find("rle%") == 0);

}

TEST(RegistryOf, smoketest) {
    using namespace tdc_algorithms;
    using ast::Value;
    using ast::Arg;
    ast::Parser p { "foo(abc, def=ghi, x = z, "
                    "jkl=mno(p, q=\"1\"), q)" };

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
        ASSERT_EQ(b[2].keyword(), "x");
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
        ASSERT_EQ(b[4].value().invokation_name(), "q");
}

TEST(RegistryOf, decl) {
    using namespace tdc_algorithms;
    decl::Algorithm b {
        "foo",
        {
            decl::Arg("a", false, "b"),
            decl::Arg("c", false, "d", ast::Value("e", {})),
            decl::Arg("f", true, "g"),
        },
        "blub",
        tdc::ds::InputRestrictionsAndFlags(tdc::io::InputRestrictions({ 0 }, true), tdc::ds::SA),
    };

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
    ASSERT_EQ(b.doc(), "blub");
    ASSERT_EQ(b.textds_flags(), tdc::ds::InputRestrictionsAndFlags(tdc::io::InputRestrictions({ 0 }, true), tdc::ds::SA));

}

TEST(RegistryOf, lookup_manual) {
    using namespace tdc_algorithms;
    Registry reg = REGISTRY;

    RegistryOf<Compressor>& cr = reg.of<Compressor>();
    RegistryOf<Generator>& gr = reg.of<Generator>();

    auto av = cr.parse_algorithm_id("rle");
    EnvRoot env1(reg, AlgorithmValue(av));
    auto c = cr.select_algorithm(env1, av);

    auto av2 = gr.parse_algorithm_id("fib(n = \"10\")");
    EnvRoot env2(reg, AlgorithmValue(av2));
    auto g = gr.select_algorithm(env2, av2);
}

TEST(RegistryOf, lookup_automatic) {
    using namespace tdc_algorithms;
    Registry reg = REGISTRY;

    RegistryOf<Compressor>& cr = reg.of<Compressor>();
    RegistryOf<Generator>& gr = reg.of<Generator>();

    auto c = cr.create_algorithm("rle");
    auto g = gr.create_algorithm("fib(n = \"10\")");
}

TEST(RegistryOf, dynamic_options) {
    using namespace tdc_algorithms;

    RegistryOf<Compressor>& r = REGISTRY.of<Compressor>();

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
            y.option("a").templated<MySub>("b");
            y.option("c").dynamic();
            y.option("d").dynamic("asdf");
            return y;
        }

        using Compressor::Compressor;
        inline virtual void decompress(Input&, Output&) {}

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

    r.register_algorithm<MyCompressor>();

    Registry reg;
    reg.register_registry(r);

    auto av = r.parse_algorithm_id("foo(x, \"qwerty\")");
    EnvRoot env(reg, AlgorithmValue(av));
    auto c = r.select_algorithm(env, av);
    std::vector<uint8_t> data;
    Output out(data);
    Input inp("test");
    c->compress(inp, out);

    ASSERT_EQ(View(data), "check");
}

TEST(TudocompDriver, all_compressors_defined) {
    using namespace tdc_algorithms;

    RegistryOf<Compressor>& r = REGISTRY.of<Compressor>();
    auto s = r.check_for_undefined_algorithms();
    bool has_undefined_compressors = s.size() > 0;
    if (has_undefined_compressors) {
        std::stringstream ss;
        for (auto& s2 : s) {
            ss << "Undefined compressor: " << s2 << "\n";
        }
        EXPECT_FALSE(has_undefined_compressors) << ss.str();
    }
}

TEST(RegistryOf, chain_sugar) {
    using namespace tdc_algorithms;
    using ast::Value;
    using ast::Arg;
    auto cmp = [](std::string sa, std::string sb) {
        ast::Parser p { sa };
        Value a = p.parse_value();
        ASSERT_EQ(a.to_string(), sb);
    };

    cmp("foo(abc, def = ghi, x = z, jkl = mno(p, q = \"1\", x = 3), q)",
        "foo(abc, def = ghi, x = z, jkl = mno(p, q = \"1\", x = \"3\"), q)");
    cmp("foo(abc, def = ghi, x = z, jkl = mno(p, q = \"1\", x = 3), q):algo2",
        "chain("
            "foo(abc, def = ghi, x = z, jkl = mno(p, q = \"1\", x = \"3\"), q)"
            ", algo2)");
    cmp("foo(abc, def = ghi, x = z, jkl = mno(p, q = \"1\", x = 3), q):algo2:algo3('asdf')",
        "chain("
            "chain("
                "foo(abc, def = ghi, x = z, jkl = mno(p, q = \"1\", x = \"3\"), q)"
                ", algo2)"
        ", algo3(\"asdf\"))");
}
