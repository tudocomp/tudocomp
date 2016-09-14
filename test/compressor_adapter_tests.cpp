#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/DecodeBuffer.hpp>
#include <tudocomp/util/View.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Algorithm.hpp>

#include <tudocomp/ChainCompressor.hpp>
#include <tudocomp/InnerNullCompressor.hpp>

#include <tudocomp_driver/Registry.hpp>

#include "test_util.h"
#include "tudocomp_test_util.h"

using namespace tudocomp;
using namespace tudocomp_algorithms;

TEST(Chain, test) {
    test::roundtrip<ChainCompressor>("aaaaaabaaaaaabaaaaaabaaaaaab",
                                     "'a','.','5','.','b',256,258,260,257,259,261,259,",
                                     R"(
                                        templated_example_compressor(
                                            debug(escape_symbol='.'),
                                        ), lzw(debug),
                                    )", REGISTRY);
}
