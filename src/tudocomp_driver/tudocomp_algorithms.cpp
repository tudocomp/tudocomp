#include <tudocomp_driver/Registry.hpp>

#include <tudocomp/ChainCompressor.hpp>
#include <tudocomp/NoopCompressor.hpp>
#include <tudocomp/InnerNullCompressor.hpp>

#include <tudocomp/lz78/Lz78DebugCoder.hpp>
#include <tudocomp/lz78/Lz78BitCoder.hpp>

#include <tudocomp/lz78/lzcics/Lz78cicsCompressor.hpp>

#include <tudocomp/misc/LCPSuffixLinkCompressor.hpp>

//compressors
#include <tudocomp/compressors/ESACompressor.hpp>
#include <tudocomp/compressors/LZ78Compressor.hpp>
#include <tudocomp/compressors/LZSSLCPCompressor.hpp>
#include <tudocomp/compressors/LZSSSlidingWindowCompressor.hpp>
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

    r.register_compressor<ESACompressor<esacomp::ESACompMaxLCP, ASCIICoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompMaxLCP, ByteCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompMaxLCP, BitOptimalCoder>>();

    r.register_compressor<ESACompressor<esacomp::ESACompBulldozer, ASCIICoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompBulldozer, ByteCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompBulldozer, BitOptimalCoder>>();

    r.register_compressor<ESACompressor<esacomp::ESACompNaive, ASCIICoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompNaive, ByteCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompNaive, BitOptimalCoder>>();

    r.register_compressor<LZ78Compressor<ASCIICoder>>();
    r.register_compressor<LZ78Compressor<ByteCoder>>();
    r.register_compressor<LZ78Compressor<BitOptimalCoder>>();

    r.register_compressor<LZWCompressor<ASCIICoder>>();
    r.register_compressor<LZWCompressor<ByteCoder>>();
    r.register_compressor<LZWCompressor<BitOptimalCoder>>();

    r.register_compressor<LZSSLCPCompressor<ASCIICoder>>();
    r.register_compressor<LZSSLCPCompressor<ByteCoder>>();
    r.register_compressor<LZSSLCPCompressor<BitOptimalCoder>>();

    r.register_compressor<LZSSSlidingWindowCompressor<ASCIICoder>>();
    r.register_compressor<LZSSSlidingWindowCompressor<ByteCoder>>();
    r.register_compressor<LZSSSlidingWindowCompressor<BitOptimalCoder>>();

    r.register_compressor< Lz78cicsCompressor<Lz78BitCoder> >();
    r.register_compressor< Lz78cicsCompressor<Lz78DebugCoder> >();

    r.register_compressor< NoopCompressor >();
}

}
