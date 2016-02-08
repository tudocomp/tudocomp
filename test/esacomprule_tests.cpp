#include <iostream>
#include <sstream>
#include <utility>
#include <algorithm>

#include "gtest/gtest.h"
#include "tudocomp.h"
#include "esacomp/esacomp_rule_compressor.h"
#include "esacomp/rule.h"

using namespace tudocomp;
using namespace esacomp;

using std::swap;

TEST(Rule, ostream) {
    std::stringstream s;
    Rule test { 0, 1, 2, };
    s << test;
    ASSERT_EQ(s.str(), "(0, 1, 2)");
}

TEST(Rules, reference) {
    Rules r { {1, 2, 3}, {4, 5, 6}, {7, 8, 9}, { 358, 288, 2 } };

    ASSERT_EQ(r[0], (Rule {1, 2, 3}));
    ASSERT_EQ(r[1], (Rule {4, 5, 6}));
    ASSERT_EQ(r[2], (Rule {7, 8, 9}));
    ASSERT_EQ(r.size(), size_t(4));

    swap(r[0], r[1]);
    ASSERT_EQ(r[0], (Rule {4, 5, 6}));
    ASSERT_EQ(r[1], (Rule {1, 2, 3}));

    std::sort(r.begin(), r.end(), rule_compare {});

    ASSERT_EQ(r[0], (Rule {1, 2, 3}));
    ASSERT_EQ(r[1], (Rule {4, 5, 6}));
    ASSERT_EQ(r[2], (Rule {7, 8, 9}));
    ASSERT_EQ(r.size(), size_t(4));

}

TEST(Rules, iterator) {
    Rules             a { {1, 2, 3}, {4, 5, 6}, {7, 8, 9}, { 358, 288, 2 } };
    std::vector<Rule> b { {1, 2, 3}, {4, 5, 6}, {7, 8, 9}, { 358, 288, 2 } };

    Rule r { 0, 0, 0 };

    {
        int ra = a.end() - a.begin();
        int rb = b.end() - b.begin();
        ASSERT_EQ(ra, rb);
    };

    {
        auto ra = a.end() - 1;
        auto rb = b.end() - 1;
        ASSERT_EQ((Rule { 358, 288, 2}), *ra);
        ASSERT_EQ((Rule { 358, 288, 2}), *rb);
    };

    {
        for (Rules::reference x : a) {
            x = r;
        }
        for (Rule& x : b) {
            x = r;
        }

        for (Rules::reference x : a) {
            ASSERT_EQ(x, r);
        }
        for (Rule& x : b) {
            ASSERT_EQ(x, r);
        }
    };

}
