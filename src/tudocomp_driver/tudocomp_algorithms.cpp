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

using lzss::DebugLZSSCoder;
using lzss::OnlineLZSSCoder;
using lzss::OfflineLZSSCoder;

/// A small helper function for directly constructing a Compressor class.
template<class C>
std::unique_ptr<Compressor> make(Env& env) {
    return std::make_unique<C>(env);
}


// All compression and encoding algorithms exposed by the command
// line interface.
//
// The format of the initializers is a tuple of three things:
// - Display name: Used in the UI for displaying the name of a algorithm
// - ID string: Used in the command line interface and the output file name
//   for identifying an algorithm
// - Description: Text describing what kind of algorithm this is.
// - Algorithm: A function pointer that constructs the base class of a
//   Compressor or Encoder.

void register_algos(Registry& r) {
    /// Define which algorithm combinations exist in the tool:

    r.algo("lz78", "Lempel-Ziv 78", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("bit", "Bit coder", "basic variable-bit-width encoding of the symbols");
            r.algo("debug", "Debug coder", "human readable, comma separated stream of (integer, char) tuples");
        });
    r.algo("lzw", "Lempel-Ziv-Welch", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("bit", "Bit coder", "basic variable-bit-width encoding of the symbols");
            r.algo("debug", "Debug coder", "human readable, comma separated stream of integers");
        });
    r.algo("lz78_cics", "Lempel-Ziv 78", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("bit", "Bit coder", "basic variable-bit-width encoding of the symbols");
            r.algo("debug", "Debug coder", "human readable, comma separated stream of integers");
        });
    r.algo("lz77ss", "Lempel-Ziv-Storer-Szymanski", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("debug", "Debug coder", "direct encoding in ASCII");
            r.algo("online", "Online factor coder", "direct encoding of factors")
                .sub_algo("Alphabet Coder", [](Registry& r) {
                    r.algo("online", "Online alphabet coder", "direct ASCII encoding of symbols");
                    r.algo("offline", "Offline alphabet coder", "optimized symbol encoding using alphabet statistics");
            });
            r.algo("offline", "Offline factor coder", "analysis of created factors and optimized encoding")
                .sub_algo("Alphabet Coder", [](Registry& r) {
                    r.algo("online", "Online alphabet coder", "direct ASCII encoding of symbols");
                    r.algo("offline", "Offline alphabet coder", "optimized symbol encoding using alphabet statistics");
            });
        });
    r.algo("lz77ss_lcp", "LZ77 Factorization using LCP", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("debug", "Debug coder", "direct encoding in ASCII");
            r.algo("online", "Online factor coder", "direct encoding of factors")
                .sub_algo("Alphabet Coder", [](Registry& r) {
                    r.algo("online", "Online alphabet coder", "direct ASCII encoding of symbols");
                    r.algo("offline", "Offline alphabet coder", "optimized symbol encoding using alphabet statistics");
            });
            r.algo("offline", "Offline factor coder", "analysis of created factors and optimized encoding")
                .sub_algo("Alphabet Coder", [](Registry& r) {
                    r.algo("online", "Online alphabet coder", "direct ASCII encoding of symbols");
                    r.algo("offline", "Offline alphabet coder", "optimized symbol encoding using alphabet statistics");
            });
        });
    r.algo("esacomp", "ESAComp", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("debug", "Debug coder", "direct encoding in ASCII");
            r.algo("online", "Online factor coder", "direct encoding of factors")
                .sub_algo("Alphabet Coder", [](Registry& r) {
                    r.algo("online", "Online alphabet coder", "direct ASCII encoding of symbols");
                    r.algo("offline", "Offline alphabet coder", "optimized symbol encoding using alphabet statistics");
            });
            r.algo("offline", "Offline factor coder", "analysis of created factors and optimized encoding")
                .sub_algo("Alphabet Coder", [](Registry& r) {
                    r.algo("online", "Online alphabet coder", "direct ASCII encoding of symbols");
                    r.algo("offline", "Offline alphabet coder", "optimized symbol encoding using alphabet statistics");
            });
        });
    r.algo("chain_test", "", "");

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

    r.compressor("lzw.debug",           make<LzwCompressor<LzwDebugCoder>>);
    r.compressor("lzw.bit",             make<LzwCompressor<LzwBitCoder>>);
    r.compressor("lz78.debug",          make<Lz78Compressor<Lz78DebugCoder>>);
    r.compressor("lz78.bit",            make<Lz78Compressor<Lz78BitCoder>>);
    r.compressor("lz78_cics.debug",     make<Lz78cicsCompressor<Lz78DebugCoder>>);
    r.compressor("lz78_cics.bit",       make<Lz78cicsCompressor<Lz78BitCoder>>);
    r.compressor("lz77ss.debug",           make<LZ77SSSlidingWindowCompressor<DebugLZSSCoder>>);
    r.compressor("lz77ss.online.online",   make<LZ77SSSlidingWindowCompressor<OnlineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("lz77ss.online.offline",  make<LZ77SSSlidingWindowCompressor<OnlineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("lz77ss.offline.online",  make<LZ77SSSlidingWindowCompressor<OfflineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("lz77ss.offline.offline", make<LZ77SSSlidingWindowCompressor<OfflineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("lz77ss_lcp.debug",           make<LZ77SSLCPCompressor<DebugLZSSCoder>>);
    r.compressor("lz77ss_lcp.online.online",   make<LZ77SSLCPCompressor<OnlineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("lz77ss_lcp.online.offline",  make<LZ77SSLCPCompressor<OnlineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("lz77ss_lcp.offline.online",  make<LZ77SSLCPCompressor<OfflineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("lz77ss_lcp.offline.offline", make<LZ77SSLCPCompressor<OfflineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("esacomp.debug",           make<LZSSESACompressor<DebugLZSSCoder>>);
    r.compressor("esacomp.online.online",   make<LZSSESACompressor<OnlineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("esacomp.online.offline",  make<LZSSESACompressor<OnlineLZSSCoder<OfflineAlphabetCoder>>>);
    r.compressor("esacomp.offline.online",  make<LZSSESACompressor<OfflineLZSSCoder<OnlineAlphabetCoder>>>);
    r.compressor("esacomp.offline.offline", make<LZSSESACompressor<OfflineLZSSCoder<OfflineAlphabetCoder>>>);

    r.compressor("chain_test", [](Env& env) {
        std::vector<CompressorConstructor> algorithms {
            make<LzwCompressor<LzwDebugCoder>>,
            make<LzwCompressor<LzwDebugCoder>>,
            make<LzwCompressor<LzwDebugCoder>>,
        };
        return std::make_unique<ChainCompressor>(env, std::move(algorithms));
    });
}

}
