#include <tudocomp_driver/Registry.hpp>

#include <tudocomp/ChainCompressor.hpp>
#include <tudocomp/NoopCompressor.hpp>
#include <tudocomp/InnerNullCompressor.hpp>

#include <tudocomp/lz78/Lz78DebugCoder.hpp>
#include <tudocomp/lz78/Lz78BitCoder.hpp>

#include <tudocomp/lz78/lzcics/Lz78cicsCompressor.hpp>

#include <tudocomp/misc/LCPSuffixLinkCompressor.hpp>

#include <tudocomp/lzss/LZSSSeanCompressor.hpp>
#include <tudocomp/lzss/LZ77SSSlidingWindowCompressor.hpp>
#include <tudocomp/lzss/LZSSESACompressor.hpp>

#include <tudocomp/lzss/DebugLZSSCoder.hpp>
#include <tudocomp/lzss/OnlineLZSSCoder.hpp>
#include <tudocomp/lzss/OfflineLZSSCoder.hpp>

#include <tudocomp/alphabet/OnlineAlphabetCoder.hpp>
#include <tudocomp/alphabet/OfflineAlphabetCoder.hpp>

//compressors
#include <tudocomp/compressors/LZ78Compressor.hpp>
#include <tudocomp/compressors/LZSSLCPCompressor.hpp>
#include <tudocomp/compressors/LZWCompressor.hpp>

//coders
#include <tudocomp/coders/ASCIICoder.hpp>
#include <tudocomp/coders/ByteCoder.hpp>
#include <tudocomp/coders/BitOptimalCoder.hpp>

#include <tudocomp/example/ExampleCompressor.hpp>

namespace tdc_algorithms {

using namespace tdc;

// Algorithm implementations
using lz78::Lz78DebugCoder;
using lz78::Lz78BitCoder;

using lz78::lzcics::Lz78cicsCompressor;

using lzss::LZ77SSSlidingWindowCompressor;
using lzss::LZSSSeanCompressor;

using lzss::LZSSESACompressor;
using lzss::ESACompBulldozer;
using lzss::ESACompMaxLCP;
using lzss::ESACompNaive;

using lzss::DebugLZSSCoder;
using lzss::OnlineLZSSCoder;
using lzss::OfflineLZSSCoder;

void register_algorithms(Registry& r);

// One global instance for the registry
Registry REGISTRY = Registry::with_all_from(register_algorithms);

#define REGISTER_COMPRESSOR_ALL_CODERS(X) \
    r.register_compressor<X<ASCIICoder>>(); \
    r.register_compressor<X<ByteCoder>>(); \
    r.register_compressor<X<BitOptimalCoder>>();

// All compression and encoding algorithms exposed by the command
// line interface.
void register_algorithms(Registry& r) {
    // Define which implementations to use for each combination.
    //
    // Because the tudocomp_driver has to select the algorithm
    // at runtime, we need to explicitly register all possible
    // template instances

    REGISTER_COMPRESSOR_ALL_CODERS(LZ78Compressor);
    REGISTER_COMPRESSOR_ALL_CODERS(LZWCompressor);
    REGISTER_COMPRESSOR_ALL_CODERS(LZSSLCPCompressor);

    r.register_compressor< Lz78cicsCompressor<Lz78BitCoder> >();
    r.register_compressor< Lz78cicsCompressor<Lz78DebugCoder> >();
    r.register_compressor< LZ77SSSlidingWindowCompressor<DebugLZSSCoder> >();
    r.register_compressor< LZ77SSSlidingWindowCompressor<OnlineLZSSCoder<OnlineAlphabetCoder>> >();
    r.register_compressor< LZ77SSSlidingWindowCompressor<OnlineLZSSCoder<OfflineAlphabetCoder>> >();
    r.register_compressor< LZ77SSSlidingWindowCompressor<OfflineLZSSCoder<OnlineAlphabetCoder>> >();
    r.register_compressor< LZ77SSSlidingWindowCompressor<OfflineLZSSCoder<OfflineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompMaxLCP, DebugLZSSCoder> >();
    r.register_compressor< LZSSESACompressor<ESACompMaxLCP, OnlineLZSSCoder<OnlineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompMaxLCP, OnlineLZSSCoder<OfflineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompMaxLCP, OfflineLZSSCoder<OnlineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompMaxLCP, OfflineLZSSCoder<OfflineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompBulldozer, DebugLZSSCoder> >();
    r.register_compressor< LZSSESACompressor<ESACompBulldozer, OnlineLZSSCoder<OnlineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompBulldozer, OnlineLZSSCoder<OfflineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompBulldozer, OfflineLZSSCoder<OnlineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompBulldozer, OfflineLZSSCoder<OfflineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompNaive, DebugLZSSCoder> >();
    r.register_compressor< LZSSESACompressor<ESACompNaive, OnlineLZSSCoder<OnlineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompNaive, OnlineLZSSCoder<OfflineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompNaive, OfflineLZSSCoder<OnlineAlphabetCoder>> >();
    r.register_compressor< LZSSESACompressor<ESACompNaive, OfflineLZSSCoder<OfflineAlphabetCoder>> >();
    r.register_compressor< LZSSSeanCompressor >();
    //broken: r.register_compressor< LCPSuffixLinkCompressor >();
    r.register_compressor< ExampleCompressor >();
    //needs new impl: r.register_compressor< TemplatedExampleCompressor<ExampleDebugCoder> >();
    //needs new impl: r.register_compressor< TemplatedExampleCompressor<ExampleBitCoder> >();

    r.register_compressor< NoopCompressor >();
}

}
