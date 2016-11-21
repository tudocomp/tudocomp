#include "tudocomp/tudocomp.hpp"
#include "tudocomp/compressors/EspCompressor.hpp"
#include "test_util.hpp"
#include <gtest/gtest.h>

using namespace tdc;

TEST(ESP, test) {
    Input i("dkascsdacjzsbkhvfaghskcbsdkcbgasdbkjcbackscfa");
    std::vector<uint8_t> v;
    Output o(v);

    auto comp = tdc::create_algo<EspCompressor>();

    comp.compress(i, o);

}
