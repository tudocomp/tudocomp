#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>

#include <gtest/gtest.h>

#include <tudocomp/lz78/factors.h>

using namespace tudocomp;
using namespace lz78;

using std::swap;

TEST(Entry, ostream1) {
    std::stringstream s;
    Entry test { 0, 'x' };
    s << test;
    ASSERT_EQ(s.str(), "(0, 'x')");
}

TEST(Entry, ostream2) {
    std::stringstream s;
    Entry test { 0, '\0' };
    s << test;
    ASSERT_EQ(s.str(), "(0, 0)");
}

TEST(Entry, ostream3) {
    std::stringstream s;
    Entry test { 0, 255 };
    s << test;
    ASSERT_EQ(s.str(), "(0, 255)");
}

TEST(Entries, reference) {
    Entries r { {1, 'a'}, {4, 'b'}, {7, '8'}, { 358, 'z' } };

    ASSERT_EQ(r[0], (Entry {1, 'a'}));
    ASSERT_EQ(r[1], (Entry {4, 'b'}));
    ASSERT_EQ(r[2], (Entry {7, '8'}));
    ASSERT_EQ(r[3], (Entry {358, 'z'}));
    ASSERT_EQ(r.size(), size_t(4));
}

TEST(Entries, iterator) {
    Entries            a { {1, 'n'}, {4, 'q'}, {7, 'a'}, { 358, 'f' } };
    std::vector<Entry> b { {1, 'n'}, {4, 'q'}, {7, 'a'}, { 358, 'f' } };

    Entry r { 0, 0 };

    {
        int ra = a.end() - a.begin();
        int rb = b.end() - b.begin();
        ASSERT_EQ(ra, rb);
    };

    {
        auto ra = a.end() - 1;
        auto rb = b.end() - 1;
        ASSERT_EQ((Entry { 358, 'f'}), *ra);
        ASSERT_EQ((Entry { 358, 'f'}), *rb);
    };

    {
        for (Entries::reference x : a) {
            x = r;
        }
        for (Entry& x : b) {
            x = r;
        }

        for (Entries::reference x : a) {
            ASSERT_EQ(x, r);
        }
        for (Entry& x : b) {
            ASSERT_EQ(x, r);
        }
    };

}
