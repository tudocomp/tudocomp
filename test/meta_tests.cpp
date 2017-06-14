#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/TypeConversion.hpp>
#include <tudocomp/meta/ast/Parser.hpp>
#include <tudocomp/meta/AlgorithmConfig.hpp>
#include <tudocomp/meta/Meta.hpp>

using namespace tdc::meta;

class LZ77Compressor {
public:
    static inline Meta meta() {
        Meta m("lz77", "compressor", "LZ77 online compressor.");
        m.sub_algo("coder", "coder"); //TODO: template parameter necessary?
        m.param("window", "10");
        m.param_list("values", "[1,4,7]");
        return m;
    }
};

class BinaryCoder {
public:
    static inline Meta meta() {
        return Meta("binary", "coder", "Binary coder.");
    }
};

TEST(Sandbox, example) {
    // registry
    AlgorithmLib lib;
    lib.emplace("lz77", LZ77Compressor::meta().decl());
    lib.emplace("binary", BinaryCoder::meta().decl());

    // parse
    DLOG(INFO) << "parse...";
    auto v = ast::Parser::parse("lz77(coder=binary)");
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
