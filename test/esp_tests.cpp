#include "tudocomp/tudocomp.hpp"
#include "tudocomp/compressors/EspCompressor.hpp"
#include "test_util.hpp"
#include <gtest/gtest.h>

using namespace tdc;

TEST(ESP, test) {
    std::vector<uint8_t> small_alpha = "abcabageheadbag"_v;
    //for (auto& e : small_alpha) { e -= 'a'; }

    // TODO: factor out recution code to be paramtric over
    // alphabet size

    std::vector<View> cases {
        "0000dkasxxxcsdacjzsbkhvfaghskcbsaaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa",
        "aaaaa",
        "asdf",
        "aaaxaaa",
        //"",
        //"a",
        "aa",
        "aaa",
        "aaaa",
        "aaaaa",
        "aaaaaa",
        //"a",
        "as",
        "asd",
        "asdf",
        "asdfg",
        "asdfgh",
        small_alpha,
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
