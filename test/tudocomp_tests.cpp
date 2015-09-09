#include <iostream>
#include <sstream>

#include "gtest/gtest.h"
#include "tudocomp.h"
#include "rule.h"

using namespace tudocomp;

TEST(Rule, ostream) {
    std::stringstream s;
    Rule test { 0, 1, 2, };
    s << test;
    ASSERT_EQ(s.str(), "(0, 1, 2)");
}
