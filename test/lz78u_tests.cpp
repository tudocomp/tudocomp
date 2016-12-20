
#include "tudocomp/tudocomp.hpp"
#include "test/util.hpp"
#include <gtest/gtest.h>

struct InputOutput {
    View in;
    View out;
};

std::ostream& operator<<(std::ostream& os, const InputOutput& v) {
    return os << v.in << " : " << v.out;
}

using namespace tdc;

TEST(Lz78U, test) {
    auto c = create_algo<LZ78UCompressor>();

    //auto i = test::compress_input("asd\0asdfgsdf"_v);
    auto i = test::compress_input("aaababaaabaababa"_v);
    auto o = test::compress_output();

    c.compress(i, o);

}
