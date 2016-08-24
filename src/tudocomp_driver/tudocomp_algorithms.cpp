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
using lzss::ESACompLinear;

using lzss::DebugLZSSCoder;
using lzss::OnlineLZSSCoder;
using lzss::OfflineLZSSCoder;

void register_algorithms(Registry& r);

// One global instance for the registry
Registry REGISTRY = Registry::with_all_from(register_algorithms);

// All compression and encoding algorithms exposed by the command
// line interface.

void register_algorithms(Registry& r) {
    // Define which implementations to use for each combination.
    //
    // Because the tudocomp_driver has to select the algorithm
    // at runtime, we need to explicitly register all possible
    // template instances

    r.compressor< LzwCompressor<LzwDebugCoder> >();
    r.compressor< LzwCompressor<LzwBitCoder> >();
    r.compressor< Lz78Compressor<Lz78BitCoder> >();
    r.compressor< Lz78Compressor<Lz78DebugCoder> >();
    r.compressor< Lz78cicsCompressor<Lz78BitCoder> >();
    r.compressor< Lz78cicsCompressor<Lz78DebugCoder> >();
    r.compressor< LZ77SSSlidingWindowCompressor<DebugLZSSCoder> >();
    r.compressor< LZ77SSSlidingWindowCompressor<OnlineLZSSCoder<OnlineAlphabetCoder>> >();
    r.compressor< LZ77SSSlidingWindowCompressor<OnlineLZSSCoder<OfflineAlphabetCoder>> >();
    r.compressor< LZ77SSSlidingWindowCompressor<OfflineLZSSCoder<OnlineAlphabetCoder>> >();
    r.compressor< LZ77SSSlidingWindowCompressor<OfflineLZSSCoder<OfflineAlphabetCoder>> >();
    r.compressor< LZ77SSLCPCompressor<DebugLZSSCoder> >();
    r.compressor< LZ77SSLCPCompressor<OnlineLZSSCoder<OnlineAlphabetCoder>> >();
    r.compressor< LZ77SSLCPCompressor<OnlineLZSSCoder<OfflineAlphabetCoder>> >();
    r.compressor< LZ77SSLCPCompressor<OfflineLZSSCoder<OnlineAlphabetCoder>> >();
    r.compressor< LZ77SSLCPCompressor<OfflineLZSSCoder<OfflineAlphabetCoder>> >();
    r.compressor< LZSSESACompressor<ESACompMaxLCP, DebugLZSSCoder> >();
    r.compressor< LZSSESACompressor<ESACompMaxLCP, OnlineLZSSCoder<OnlineAlphabetCoder>> >();
    r.compressor< LZSSESACompressor<ESACompMaxLCP, OnlineLZSSCoder<OfflineAlphabetCoder>> >();
    r.compressor< LZSSESACompressor<ESACompMaxLCP, OfflineLZSSCoder<OnlineAlphabetCoder>> >();
    r.compressor< LZSSESACompressor<ESACompMaxLCP, OfflineLZSSCoder<OfflineAlphabetCoder>> >();
    r.compressor< LZSSESACompressor<ESACompLinear, DebugLZSSCoder> >();
    r.compressor< LZSSESACompressor<ESACompLinear, OnlineLZSSCoder<OnlineAlphabetCoder>> >();
    r.compressor< LZSSESACompressor<ESACompLinear, OnlineLZSSCoder<OfflineAlphabetCoder>> >();
    r.compressor< LZSSESACompressor<ESACompLinear, OfflineLZSSCoder<OnlineAlphabetCoder>> >();
    r.compressor< LZSSESACompressor<ESACompLinear, OfflineLZSSCoder<OfflineAlphabetCoder>> >();
}

}
