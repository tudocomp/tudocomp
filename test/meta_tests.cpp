#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/TypeConversion.hpp>
#include <tudocomp/meta/ast/Parser.hpp>
#include <tudocomp/meta/AlgorithmConfig.hpp>

using namespace tdc::meta;

TEST(Sandbox, example) {
    auto lz77 = AlgorithmDecl("lz77", "compressor", "LZ77 online compressor.");
    lz77.add_param(AlgorithmDecl::Param("window", true, false, "",
        ast::Parser::parse("10")));
    lz77.add_param(AlgorithmDecl::Param("coder", false, false, "coder",
        ast::Parser::parse("binary()")));
    lz77.add_param(AlgorithmDecl::Param("values", true, true, "",
        ast::Parser::parse("[1,4,7]")));
    //DLOG(INFO) << lz77.str();

    auto binary = AlgorithmDecl("binary", "coder", "Binary coder.");
    //DLOG(INFO) << binary.str();

    // registry
    AlgorithmLib lib;
    lib.emplace("lz77", std::move(lz77));
    lib.emplace("binary", std::move(binary));

    // parse
    DLOG(INFO) << "parse...";
    auto v = ast::Parser::parse("lz77");
    DLOG(INFO) << v->str();

    // attempt to create config
    DLOG(INFO) << "config...";
    auto cfg = AlgorithmConfig(lib.at("lz77"), v, lib);
    DLOG(INFO) << cfg.str();

    // signature
    DLOG(INFO) << "signature...";
    DLOG(INFO) << cfg.signature();

    // stuff
    DLOG(INFO) << "get...";
    auto window = cfg.get_int("window");
    auto values = cfg.get_vector<int>("values");
    DLOG(INFO) << "window=" << window << ", values.size()=" << values.size();

    auto& coder_cfg = cfg.get_sub_config("coder");
    DLOG(INFO) << "coder: " << coder_cfg.str();
}
