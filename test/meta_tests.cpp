#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/util/meta/Mockup.hpp>

using namespace tdc;

TEST(Sandbox, example) {
    auto lz77 = meta::Algorithm("lz77", "compressor", "LZ77 online compressor.");
    lz77.add_param(meta::Param("window"));
    lz77.add_param(meta::Param("coder", false, false, "coder"));
    DLOG(INFO) << lz77.str();

    auto binary = meta::Algorithm("binary", "coder", "Binary coder.");
    DLOG(INFO) << binary.str();

    // registry
    meta::AlgorithmMap map;
    map.emplace("lz77", std::move(lz77));
    map.emplace("binary", std::move(binary));

    // parse
    meta::parse("lz77(window=10, coder=binary)", map);
}

