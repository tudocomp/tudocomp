#include "tudocomp/tudocomp.hpp"
#include "tudocomp/compressors/EspCompressor.hpp"
#include "test_util.hpp"
#include <gtest/gtest.h>

using namespace tdc;

TEST(ESP, test) {
    std::vector<View> cases {
        "0000dkasxxxcsdacjzsbkhvfaghskcbsaaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa",
        "aaaaa",
        "asdf",
        "aaaxaaa",
        "",
        "a",
        "aa",
        "aaa",
        "aaaa",
        "aaaaa",
        "aaaaaa",
        "a",
        "as",
        "asd",
        "asdf",
        "asdfg",
        "asdfgh",
    };

    for (auto& c : cases) {
        Input i(c);
        std::vector<uint8_t> v;
        Output o(v);

        auto comp = tdc::create_algo<EspCompressor>();

        comp.compress(i, o);

        std::cout << "\n";
    }

}
