#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/TypeConversion.hpp>
#include <tudocomp/meta/ast/Parser.hpp>
#include <tudocomp/meta/AlgorithmConfig.hpp>
#include <tudocomp/meta/Meta.hpp>

using namespace tdc::meta;

class BinaryCoder {
public:
    static inline Meta meta() {
        return Meta("binary", "coder", "Binary coder.");
    }
};

class LZ77Compressor {
public:
    static inline Meta meta() {
        Meta m("lz77", "compressor", "LZ77 online compressor.");
        m.sub_algo<BinaryCoder>("coder", "coder");
        m.param("window", "10");
        m.param_list("values", "[1,4,7]");
        return m;
    }
};

class Something {
public:
    static inline Meta meta() {
        Meta m("sth", "highlevel", "Something high-level.");
        m.sub_algo<LZ77Compressor>("compressor", "compressor");
        return m;
    }
};

TEST(Sandbox, example) {
    // registry
    AlgorithmLib lib;
    lib.emplace("binary", BinaryCoder::meta().decl());
    lib.emplace("lz77", LZ77Compressor::meta().decl());
    lib.emplace("sth", Something::meta().decl());

    // parse
    DLOG(INFO) << "parse...";
    auto v = ast::Parser::parse("sth()");
    DLOG(INFO) << v->str();

    // attempt to create config
    DLOG(INFO) << "config...";
    auto cfg = AlgorithmConfig(lib.at("sth"), v, lib);
    DLOG(INFO) << cfg.str();

    // signature
    DLOG(INFO) << "signature...";
    DLOG(INFO) << cfg.signature();
}
