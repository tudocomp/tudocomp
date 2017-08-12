#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/meta/AlgorithmDecl.hpp>
#include <tudocomp/meta/ast/TypeConversion.hpp>
#include <tudocomp/meta/ast/Parser.hpp>
#include <tudocomp/meta/AlgorithmConfig.hpp>
#include <tudocomp/meta/Meta.hpp>
#include <tudocomp/meta/Registry.hpp>

using namespace tdc::meta;

constexpr TypeDesc coder_td("coder");
constexpr TypeDesc compressor_td("coder");

class Algorithm {};

class OtherAlgo : public Algorithm {
public:
    static inline Meta meta() {
        return Meta("other", TypeDesc("other"), "Any other algorithm.");
    }
};

class BinaryCoder : public Algorithm {
public:
    static inline Meta meta() {
        return Meta("binary", coder_td, "Binary coder.");
    }
};

class UnaryCoder : public Algorithm {
public:
    static inline Meta meta() {
        return Meta("unary", coder_td, "Unary coder.");
    }
};

template<typename coder_t, typename coder2_t>
class LZ77Compressor : public Algorithm {
public:
    static inline Meta meta() {
        Meta m("lz77", compressor_td, "LZ77 online compressor.");
        m.param("coder").strategy<coder_t>(coder_td, Meta::Default<BinaryCoder>());
        m.param("coders").strategy_list<coder_t, coder2_t>(
            coder_td, Meta::Defaults<BinaryCoder, UnaryCoder>());
        m.param("window").primitive(10);
        m.param("values").primitive_list({1,4,7});
        return m;
    }
};

TEST(Sandbox, example) {
    Registry<Algorithm> registry;
    registry.register_algorithm<LZ77Compressor<BinaryCoder, BinaryCoder>>();
    registry.register_algorithm<LZ77Compressor<BinaryCoder, UnaryCoder>>();
    registry.register_algorithm<LZ77Compressor<UnaryCoder, BinaryCoder>>();
    registry.register_algorithm<LZ77Compressor<UnaryCoder, UnaryCoder>>();

    auto algo = registry.select("lz77(window=147)");

    /*
    // parse
    DLOG(INFO) << "parse...";
    auto v = ast::Parser::parse("lz77()");
    DLOG(INFO) << v->str();

    // attempt to create config
    DLOG(INFO) << "config...";
    auto cfg = AlgorithmConfig(lib.at("lz77"), v, lib);
    DLOG(INFO) << cfg.str();

    // signature
    DLOG(INFO) << "signature...";
    DLOG(INFO) << cfg.signature()->str();
    */
}
