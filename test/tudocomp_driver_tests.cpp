#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include <tudocomp/util.hpp>
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
    std::string text = "asdfghjklöä";
    bool abort = false;
    // Without header
    driver_test::roundtrip("lz78(ascii)", "_header_test_0", text, true, abort, true).check();

    // With header
    driver_test::roundtrip("lz78(ascii)", "_header_test_1", text, false, abort, true).check();

    ASSERT_FALSE(abort);

    constexpr auto expected_magic =
        "lz78(coder=ascii())%";

    std::string text0 = test::read_test_file(driver_test::roundtrip_comp_file_name(
        "lz78(ascii)", "_header_test_0"));

    ASSERT_FALSE(text0.find(expected_magic) == 0);

    std::string text1 = test::read_test_file(driver_test::roundtrip_comp_file_name(
        "lz78(ascii)", "_header_test_1"));

    ASSERT_TRUE(text1.find(expected_magic) == 0);

}
