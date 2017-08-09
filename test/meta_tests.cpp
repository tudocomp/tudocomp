#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/TypeConversion.hpp>
#include <tudocomp/meta/ast/Parser.hpp>
#include <tudocomp/meta/AlgorithmConfig.hpp>
#include <tudocomp/meta/Meta.hpp>

using namespace tdc::meta;

constexpr TypeDesc coder_td("coder");
constexpr TypeDesc compressor_td("coder");

class BinaryCoder {
public:
    static inline Meta meta() {
        return Meta("binary", TypeDesc("xcoder", coder_td), "Binary coder.");
    }
};

class LZ77Compressor {
public:
    static inline Meta meta() {
        Meta m("lz77", compressor_td, "LZ77 online compressor.");
        m.sub_algo<BinaryCoder>("coder", coder_td);
        m.param("window", "10");
        m.param_list("values", "[1,4,7]");
        return m;
    }
};

TEST(Sandbox, example) {
    // registry
    AlgorithmLib lib;
    lib.emplace("binary", BinaryCoder::meta().decl());
    lib.emplace("lz77", LZ77Compressor::meta().decl());

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
}
