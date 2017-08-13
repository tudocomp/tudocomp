#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/meta/Registry.hpp>
#include <tudocomp/Algorithm.hpp>

using namespace tdc;

constexpr TypeDesc coder_td("coder");
constexpr TypeDesc compressor_td("compressor");

class OtherAlgo : public Algorithm {
public:
    static inline Meta meta() {
        return Meta("other", TypeDesc("other"), "Any other algorithm.");
    }

    using Algorithm::Algorithm;
};

class BinaryCoder : public Algorithm {
public:
    static inline Meta meta() {
        return Meta("binary", coder_td, "Binary coder.");
    }

    using Algorithm::Algorithm;
};

class UnaryCoder : public Algorithm {
public:
    static inline Meta meta() {
        return Meta("unary", coder_td, "Unary coder.");
    }

    using Algorithm::Algorithm;
};

template<typename coder_t, typename coder2_t>
class LZ77Compressor : public Algorithm {
public:
    static inline Meta meta() {
        Meta m("lz77", compressor_td, "LZ77 online compressor.");
        m.param("coder").strategy<coder_t>(coder_td, Meta::Default<UnaryCoder>());
        m.param("coders").strategy_list<coder_t, coder2_t>(
            coder_td, Meta::Defaults<UnaryCoder, UnaryCoder>());
        m.param("window").primitive(10);
        m.param("values").primitive_list({1,4,7});
        return m;
    }

    using Algorithm::Algorithm;
};

TEST(Sandbox, example) {
    Registry<Algorithm> registry;
    registry.register_algorithm<LZ77Compressor<BinaryCoder, BinaryCoder>>();
    registry.register_algorithm<LZ77Compressor<BinaryCoder, UnaryCoder>>();
    registry.register_algorithm<LZ77Compressor<UnaryCoder, BinaryCoder>>();
    registry.register_algorithm<LZ77Compressor<UnaryCoder, UnaryCoder>>();

    auto algo = registry.select("lz77(window=147)");
    DLOG(INFO) << "instance: " << algo.get();

    //auto algo2 = Algorithm::instance<LZ77Compressor<BinaryCoder, BinaryCoder>>();

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
