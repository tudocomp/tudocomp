#include <tudocomp_driver/registry.h>

#include <tudocomp/lz78/Lz78Compressor.hpp>
#include <tudocomp/lz78/Lz78DebugCoder.hpp>
#include <tudocomp/lz78/Lz78BitCoder.hpp>

#include <tudocomp/lzw/LzwCompressor.hpp>
#include <tudocomp/lzw/LzwDebugCoder.hpp>
#include <tudocomp/lzw/LzwBitCoder.hpp>

#include <tudocomp/lz78/lzcics/Lz78cicsCompressor.hpp>

#include <tudocomp/lzss/LZ77SSSlidingWindowCompressor.hpp>
#include <tudocomp/lzss/LZ77SSLCPCompressor.hpp>
#include <tudocomp/lzss/LZSSESACompressor.hpp>

#include <tudocomp/lzss/DebugLZSSCoder.hpp>
#include <tudocomp/lzss/OnlineLZSSCoder.hpp>
#include <tudocomp/lzss/OfflineLZSSCoder.hpp>

#include <tudocomp/alphabet/OnlineAlphabetCoder.hpp>
#include <tudocomp/alphabet/OfflineAlphabetCoder.hpp>

#include <tudocomp/ChainCompressor.hpp>
#include <tudocomp/lzss/LZ77TestCompressor.hpp>

namespace tudocomp_driver {

using namespace tudocomp;

// Algorithm implementations
using lzw::LzwCompressor;
using lzw::LzwDebugCoder;
using lzw::LzwBitCoder;

using lz78::Lz78Compressor;
using lz78::Lz78DebugCoder;
using lz78::Lz78BitCoder;

using lz78::lzcics::Lz78cicsCompressor;

using lzss::LZ77SSSlidingWindowCompressor;
using lzss::LZ77SSLCPCompressor;

using lzss::LZSSESACompressor;
using lzss::ESACompBulldozer;
using lzss::ESACompMaxLCP;

using lzss::DebugLZSSCoder;
using lzss::OnlineLZSSCoder;
using lzss::OfflineLZSSCoder;

using lzss::LZ77TestCompressor;

/// A small helper function for directly constructing a Compressor class.
template<class C>
std::unique_ptr<Compressor> make(Env& env) {
    return std::make_unique<C>(env);
}

// All compression and encoding algorithms exposed by the command
// line interface.

void register_algorithms(Registry& r) {
    // Register the main compression algorithms
    auto compressor = r.type("compressor");
    compressor.regist(R"(
        lz78(coder:     static lz78_coder = bit,
             dict_size:        string     = "inf"))",

        "Lempel-Ziv 78 [...]"
    );
    compressor.regist("lz78_cics(coder: static lz78_coder = bit)");
    compressor.regist("lzw(coder: static lzw_coder = bit)",
        "Lempel-Ziv-Welch");
    compressor.regist("lz77ss(coder: static lzss_coder)",
        "Lempel-Ziv-Storer-Szymanski");
    compressor.regist("lz77ss_lcp(coder: static lzss_coder = online)",
        "LZ77 Factorization using LCP");
    compressor.regist("esacomp(coder: static lzss_coder = online)",
        "ESAComp");
    compressor.regist("chain_test()");

    // Register all subalgorithms
    {
        auto coder = r.type("lz78_coder");
        coder.regist("bit()",
            "Bit coder\n"
            "Basic variable-bit-width encoding of the symbols");
        coder.regist("debug()",
            "Debug coder\n"
            "Human readable, comma separated "
            "stream of (integer, char) tuples");
    }
    {
        auto coder = r.type("lzw_coder");
        coder.regist("bit()",
            "Bit coder\n"
            "Basic variable-bit-width encoding of the symbols");
        coder.regist("debug()",
            "Debug coder\n"
            "Human readable, comma separated stream of integers");
    }
    {
        auto coder = r.type("lzss_coder");
        coder.regist("debug()",
            "Debug coder\n"
            "Direct encoding in ASCII");
        coder.regist("online(alphabet_coder: static alpha_coder = online)",
            "Online factor coder\n"
            "Direct encoding of factors");
        coder.regist("offline(alphabet_coder: static alpha_coder = online)",
            "Offline factor coder\n"
            "Analysis of created factors and optimized encoding");
    }
    {
        auto coder = r.type("alpha_coder");
        coder.regist("online()",
            "Online alphabet coder\n"
            "Direct ASCII encoding of symbols");
        coder.regist("offline()",
            "Offline alphabet coder\n"
            "Optimized symbol encoding using alphabet statistics");
    }

    // Define which implementations to use for each combination:

    // compressor() takes a function that constructs the right
    // Compressor with an Env& argument. You could use a lambda like this:
    //
    //     r.compressor("lzw.debug", [](Env& env) {
    //         return std::make_unique<LzwCompressor<LzwDebugCoder>>(env);
    //     });
    //
    // But since most of the algorithms take no additional constructor
    // arguments we use the make() helper function to make this less verbose.

    r.compressor("lzw(debug())",           make<LzwCompressor<LzwDebugCoder>>);
    r.compressor("lzw(bit())",             make<LzwCompressor<LzwBitCoder>>);
    r.compressor("lz78(bit())",            make<Lz78Compressor<Lz78BitCoder>>);
    r.compressor("lz78(debug())",          make<Lz78Compressor<Lz78DebugCoder>>);
    r.compressor("lz78_cics(bit())",       make<Lz78cicsCompressor<Lz78BitCoder>>);
    r.compressor("lz78_cics(debug())",     make<Lz78cicsCompressor<Lz78DebugCoder>>);
    r.compressor("lz77ss(debug())",            make<LZ77SSSlidingWindowCompressor<DebugLZSSCoder>>);
    r.compressor("lz77ss(online(online()))",   make<LZ77SSSlidingWindowCompressor<OnlineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("lz77ss(online(offline()))",  make<LZ77SSSlidingWindowCompressor<OnlineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("lz77ss(offline(online()))",  make<LZ77SSSlidingWindowCompressor<OfflineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("lz77ss(offline(offline()))", make<LZ77SSSlidingWindowCompressor<OfflineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("lz77ss_lcp(debug())",            make<LZ77SSLCPCompressor<DebugLZSSCoder>>);
    r.compressor("lz77ss_lcp(online(online()))",   make<LZ77SSLCPCompressor<OnlineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("lz77ss_lcp(online(offline()))",  make<LZ77SSLCPCompressor<OnlineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("lz77ss_lcp(offline(online()))",  make<LZ77SSLCPCompressor<OfflineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("lz77ss_lcp(offline(offline()))", make<LZ77SSLCPCompressor<OfflineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("esacomp(debug())",            make<LZSSESACompressor<ESACompMaxLCP, DebugLZSSCoder>>);
    r.compressor("esacomp(online(online()))",   make<LZSSESACompressor<ESACompMaxLCP, OnlineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("esacomp(online(offline()))",  make<LZSSESACompressor<ESACompMaxLCP, OnlineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("esacomp(offline(online()))",  make<LZSSESACompressor<ESACompMaxLCP, OfflineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("esacomp(offline(offline()))", make<LZSSESACompressor<ESACompMaxLCP, OfflineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("LZ77TestCompressor",           make<LZ77TestCompressor>);

    r.compressor("chain_test()", [](Env& env) {
        std::vector<CompressorConstructor> algorithms {
            make<LzwCompressor<LzwDebugCoder>>,
            make<LzwCompressor<LzwDebugCoder>>,
            make<LzwCompressor<LzwDebugCoder>>,
        };
        return std::make_unique<ChainCompressor>(env, std::move(algorithms));
    });
}

}
