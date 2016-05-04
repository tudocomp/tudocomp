#include <stdio.h>
#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include <tudocomp/util.h>

#include "test_util.h"
#include "tudocomp_driver/registry.h"
#include "tudocomp_driver/AlgorithmStringParser.hpp"

#include "tudocomp_driver_util.h"

TEST(TudocompDriver, list) {
    // Test that we got at least the amount of algorithms
    // we had when writing this test.

    //TODO this test does not make any sense this way...
    //TODO should somehow compare against registry instead

    /*
    auto list = list::tudocomp_list();

    ASSERT_EQ(list.header, "This build supports the following algorithms:");

    auto& root = list.root;

    ASSERT_GE(root.algos.size(), 2u);

    {
        auto& r0 = root.algos[0];
        ASSERT_GE(r0.subalgos.size(), 2u);
        {
            auto& r00 = r0.subalgos[0];
            ASSERT_GE(r00.algos.size(), 4u);
        }
        {
            auto& r01 = r0.subalgos[1];
            ASSERT_GE(r01.algos.size(), 4u);
        }
    }
    {
        auto& r1 = root.algos[1];
        ASSERT_GE(r1.subalgos.size(), 1u);
        {
            auto& r10 = r1.subalgos[0];
            ASSERT_GE(r10.algos.size(), 1u);
        }
    }
    */
}

TEST(TudocompDriver, algorithm_header) {
    std::string text = "asdfghjklöä";
    bool abort = false;
    // Without header
    roundtrip("lz78.debug", "_header_test_0", text, true, abort);

    // With header
    roundtrip("lz78.debug", "_header_test_1", text, false, abort);

    ASSERT_FALSE(abort);

    std::string text0 = read_test_file(roundtrip_comp_file_name(
        "lz78.debug", "_header_test_0"));

    ASSERT_TRUE(text0.find('(') == 0);

    std::string text1 = read_test_file(roundtrip_comp_file_name(
        "lz78.debug", "_header_test_1"));

    ASSERT_TRUE(text1.find("lz78.debug%(") == 0);

}

TEST(NewAlgorithmStringParser, smoketest) {
    using namespace tudocomp_driver;
    Parser p { "foo(abc, def=ghi, jkl=mno(p, q=1))" };

    auto x = p.parse().unwrap();

    ASSERT_EQ(x.name, "foo");
    ASSERT_EQ(x.args.size(), 3);
    ASSERT_EQ(x.args[0].keyword, "");
    ASSERT_EQ(x.args[0].get<std::string>(), "abc");
    ASSERT_EQ(x.args[1].keyword, "def");
    ASSERT_EQ(x.args[1].get<std::string>(), "ghi");
    ASSERT_EQ(x.args[2].keyword, "jkl");
    ASSERT_EQ(x.args[2].get<AlgorithmSpec>().name, "mno");
    auto y = x.args[2].get<AlgorithmSpec>().args;
    ASSERT_EQ(y.size(), 2);
    ASSERT_EQ(y[0].keyword, "");
    ASSERT_EQ(y[0].get<std::string>(), "p");
    ASSERT_EQ(y[1].keyword, "q");
    ASSERT_EQ(y[1].get<std::string>(), "1");
}

TEST(RegistryV3, test) {
    using namespace tudocomp_driver;
    RegistryV3 r;
    register2(r);

    auto print = [](std::vector<AlgorithmSpecBuilder>& x, size_t iden) {
        std::vector<std::string> cells;

        for (auto& y : x) {
            auto spec = y.m_spec.to_string();

            std::stringstream where;
            bool first = true;
            for (auto& z : y.m_arg_ids) {
                if (first) {
                    where << "\n  where ";
                } else {
                    where << "\n        ";
                }
                first = false;
                where << "`" << z.first << "` is one of [" << z.second.first << "],";
            }
            auto s = spec + where.str();
            if (y.m_arg_ids.size() > 0) {
                s = s.substr(0, s.size() - 1);
            }
            cells.push_back(s);
            cells.push_back(y.m_doc);
        }

        std::cout << indent_lines(make_table(cells, 2), iden) << "\n\n";
    };

    std::cout << "[Compression algorithms]\n";
    print(r.m_algorithms["compressor"], 0);

    std::cout << "[Argument types]\n";
    for (auto& x : r.m_algorithms) {
        if (x.first == "compressor") {
            continue;
        }
        std::cout << "  [" << x.first << "]\n";
        print(x.second, 2);
    }

    r.check_for_undefined_compressors();
}
