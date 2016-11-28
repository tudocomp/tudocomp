#include <tudocomp/config.h>
#include <tudocomp_driver/Registry.hpp>

#include <tudocomp/ChainCompressor.hpp>
#include <tudocomp/NoopCompressor.hpp>
#include <tudocomp/InnerNullCompressor.hpp>
#include <tudocomp/LiteralEncoder.hpp>

//compressors
#include <tudocomp/compressors/ESACompressor.hpp>
#include <tudocomp/compressors/LZ78Compressor.hpp>
#include <tudocomp/compressors/LZSSLCPCompressor.hpp>
#include <tudocomp/compressors/LZSSSlidingWindowCompressor.hpp>
#include <tudocomp/compressors/LZWCompressor.hpp>
#include <tudocomp/compressors/RePairCompressor.hpp>
#include <tudocomp/compressors/RunLengthEncoder.hpp>
#include <tudocomp/compressors/EasyRLECompressor.hpp>
#include <tudocomp/compressors/MTFCompressor.hpp>
#include <tudocomp/compressors/BWTCompressor.hpp>

//coders
#include <tudocomp/coders/ASCIICoder.hpp>
#include <tudocomp/coders/ByteCoder.hpp>
#include <tudocomp/coders/BitOptimalCoder.hpp>
#include <tudocomp/coders/VariantCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>


//lz78 tries
#ifdef JUDY_H_AVAILABLE
	#include <tudocomp/compressors/lz78/JudyTrie.hpp>
#endif
#include <tudocomp/compressors/lz78/HashTrie.hpp>
#include <tudocomp/compressors/lz78/BinaryTrie.hpp>
#include <tudocomp/compressors/lz78/BinarySortedTrie.hpp>
#include <tudocomp/compressors/lz78/MyHashTrie.hpp>


namespace tdc_algorithms {

using namespace tdc;

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

//LZ78

    r.register_compressor<LZ78Compressor<ASCIICoder     , lz78::BinarySortedTrie>>();
    r.register_compressor<LZ78Compressor<ByteCoder      , lz78::BinarySortedTrie>>();
    r.register_compressor<LZ78Compressor<BitOptimalCoder, lz78::BinarySortedTrie>>();
    r.register_compressor<LZWCompressor <ASCIICoder     , lz78::BinarySortedTrie>>();
    r.register_compressor<LZWCompressor <ByteCoder      , lz78::BinarySortedTrie>>();
    r.register_compressor<LZWCompressor <BitOptimalCoder, lz78::BinarySortedTrie>>();
	
    r.register_compressor<LZ78Compressor<ASCIICoder     , lz78::MyHashTrie>>();
    r.register_compressor<LZ78Compressor<ByteCoder      , lz78::MyHashTrie>>();
    r.register_compressor<LZ78Compressor<BitOptimalCoder, lz78::MyHashTrie>>();
    r.register_compressor<LZWCompressor <ASCIICoder     , lz78::MyHashTrie>>();
    r.register_compressor<LZWCompressor <ByteCoder      , lz78::MyHashTrie>>();
    r.register_compressor<LZWCompressor <BitOptimalCoder, lz78::MyHashTrie>>();
	
#ifdef JUDY_H_AVAILABLE
    r.register_compressor<LZ78Compressor<ASCIICoder     , lz78::JudyTrie>>();
    r.register_compressor<LZ78Compressor<ByteCoder      , lz78::JudyTrie>>();
    r.register_compressor<LZ78Compressor<BitOptimalCoder, lz78::JudyTrie>>();
    r.register_compressor<LZWCompressor <ASCIICoder     , lz78::JudyTrie>>();
    r.register_compressor<LZWCompressor <ByteCoder      , lz78::JudyTrie>>();
    r.register_compressor<LZWCompressor <BitOptimalCoder, lz78::JudyTrie>>();
#endif


    r.register_compressor<LZ78Compressor<ASCIICoder     , lz78::TernaryTrie>>();
    r.register_compressor<LZ78Compressor<ByteCoder      , lz78::TernaryTrie>>();
    r.register_compressor<LZ78Compressor<BitOptimalCoder, lz78::TernaryTrie>>();
    r.register_compressor<LZWCompressor <ASCIICoder     , lz78::TernaryTrie>>();
    r.register_compressor<LZWCompressor <ByteCoder      , lz78::TernaryTrie>>();
    r.register_compressor<LZWCompressor <BitOptimalCoder, lz78::TernaryTrie>>();

    r.register_compressor<LZ78Compressor<ASCIICoder     , lz78::HashTrie>>();
    r.register_compressor<LZ78Compressor<ByteCoder      , lz78::HashTrie>>();
    r.register_compressor<LZ78Compressor<BitOptimalCoder, lz78::HashTrie>>();
    r.register_compressor<LZWCompressor <ASCIICoder     , lz78::HashTrie>>();
    r.register_compressor<LZWCompressor <ByteCoder      , lz78::HashTrie>>();
    r.register_compressor<LZWCompressor <BitOptimalCoder, lz78::HashTrie>>();

    r.register_compressor<LZ78Compressor<ASCIICoder     , lz78::BinaryTrie>>();
    r.register_compressor<LZ78Compressor<ByteCoder      , lz78::BinaryTrie>>();
    r.register_compressor<LZ78Compressor<BitOptimalCoder, lz78::BinaryTrie>>();
    r.register_compressor<LZWCompressor <ASCIICoder     , lz78::BinaryTrie>>();
    r.register_compressor<LZWCompressor <ByteCoder      , lz78::BinaryTrie>>();
    r.register_compressor<LZWCompressor <BitOptimalCoder, lz78::BinaryTrie>>();



    r.register_compressor<RePairCompressor<ASCIICoder>>();
    r.register_compressor<RePairCompressor<ByteCoder>>();
    r.register_compressor<RePairCompressor<BitOptimalCoder>>();
    r.register_compressor<RePairCompressor<HuffmanCoder>>();

    r.register_compressor<LiteralEncoder<ASCIICoder>>();
    r.register_compressor<LiteralEncoder<ByteCoder>>();
    r.register_compressor<LiteralEncoder<BitOptimalCoder>>();
    r.register_compressor<LiteralEncoder<HuffmanCoder>>();

    r.register_compressor<ESACompressor<esacomp::ESACompMaxLCP, ASCIICoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompMaxLCP, ByteCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompMaxLCP, BitOptimalCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompMaxLCP, HuffmanCoder>>();

    r.register_compressor<ESACompressor<esacomp::ESACompBulldozer, ASCIICoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompBulldozer, ByteCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompBulldozer, BitOptimalCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompBulldozer, HuffmanCoder>>();

    r.register_compressor<ESACompressor<esacomp::ESACompNaive, ASCIICoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompNaive, ByteCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompNaive, BitOptimalCoder>>();
    r.register_compressor<ESACompressor<esacomp::ESACompNaive, HuffmanCoder>>();

    r.register_compressor<LZSSLCPCompressor<ASCIICoder>>();
    r.register_compressor<LZSSLCPCompressor<ByteCoder>>();
    r.register_compressor<LZSSLCPCompressor<BitOptimalCoder>>();
    r.register_compressor<LZSSLCPCompressor<HuffmanCoder>>();

    // [!] causes infinite loop in tdc::Registry::all_algorithms_with_static
    // r.register_compressor<LZSSLCPCompressor<VariantCoder<ASCIICoder, ByteCoder>>>();

    r.register_compressor<LZSSSlidingWindowCompressor<ASCIICoder>>();
    r.register_compressor<LZSSSlidingWindowCompressor<ByteCoder>>();
    r.register_compressor<LZSSSlidingWindowCompressor<BitOptimalCoder>>();
    r.register_compressor<LZSSSlidingWindowCompressor<HuffmanCoder>>();

    r.register_compressor<RunLengthEncoder<ASCIICoder>>();
    r.register_compressor<RunLengthEncoder<ByteCoder>>();
    r.register_compressor<RunLengthEncoder<BitOptimalCoder>>();
    r.register_compressor<RunLengthEncoder<HuffmanCoder>>();

    r.register_compressor< EasyRLECompressor>();
    r.register_compressor< MTFCompressor>();
    r.register_compressor< BWTCompressor>();

    r.register_compressor< ChainCompressor >();
    r.register_compressor< NoopCompressor >();
}

}
