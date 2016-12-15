#include <tudocomp/tudocomp.hpp>
#include <tudocomp_driver/Registry.hpp>

namespace tdc_algorithms {

using namespace tdc;

void register_algorithms(Registry& r);

// One global instance for the registry
Registry REGISTRY = Registry::with_all_from(register_algorithms);

#define REGISTER_WITH_ONLINE_CODERS(C) \
    r.register_compressor<C<ASCIICoder>>();\
    r.register_compressor<C<BitCoder>>();\
    r.register_compressor<C<EliasGammaCoder>>();\
    r.register_compressor<C<EliasDeltaCoder>>();

#define REGISTER_WITH_ONLINE_CODERS_M(C,T) \
    r.register_compressor<C<ASCIICoder, T>>();\
    r.register_compressor<C<BitCoder, T>>();\
    r.register_compressor<C<EliasGammaCoder, T>>();\
    r.register_compressor<C<EliasDeltaCoder, T>>();

#define REGISTER_WITH_OFFLINE_CODERS(C) \
    r.register_compressor<C<Code2Coder>>();\
    r.register_compressor<C<HuffmanCoder>>();

#define REGISTER_WITH_OFFLINE_CODERS_M(C,T) \
    r.register_compressor<C<Code2Coder, T>>();\
    r.register_compressor<C<HuffmanCoder, T>>();

#define REGISTER_WITH_ALL_CODERS(C) \
    REGISTER_WITH_ONLINE_CODERS(C);\
    REGISTER_WITH_OFFLINE_CODERS(C);

#define REGISTER_WITH_ALL_CODERS_M(C,T) \
    REGISTER_WITH_ONLINE_CODERS_M(C,T);\
    REGISTER_WITH_OFFLINE_CODERS_M(C,T);

// All compression and encoding algorithms exposed by the command
// line interface.
void register_algorithms(Registry& r) {
    // Define which implementations to use for each combination.
    //
    // Because the tudocomp_driver has to select the algorithm
    // at runtime, we need to explicitly register all possible
    // template instances

    REGISTER_WITH_ALL_CODERS(LiteralEncoder);

    REGISTER_WITH_ONLINE_CODERS_M(LZ78Compressor, lz78::BinarySortedTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZ78Compressor, lz78::BinaryTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZ78Compressor, lz78::HashTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZ78Compressor, lz78::MyHashTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZ78Compressor, lz78::TernaryTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZ78Compressor, lz78::CedarTrie);

    REGISTER_WITH_ONLINE_CODERS_M(LZWCompressor, lz78::BinarySortedTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZWCompressor, lz78::BinaryTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZWCompressor, lz78::HashTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZWCompressor, lz78::MyHashTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZWCompressor, lz78::TernaryTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZWCompressor, lz78::CedarTrie);

#ifdef JUDY_H_AVAILABLE
    REGISTER_WITH_ONLINE_CODERS_M(LZ78Compressor, lz78::JudyTrie);
    REGISTER_WITH_ONLINE_CODERS_M(LZWCompressor, lz78::JudyTrie);
#endif

    REGISTER_WITH_ALL_CODERS(RePairCompressor);

    REGISTER_WITH_ALL_CODERS(LZSSLCPCompressor);


    r.register_compressor<ESACompressor<Code2Coder, esacomp::MaxHeapStrategy,  esacomp::MarvinBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::MaxLCPStrategy,   esacomp::MarvinBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::LazyListStrategy, esacomp::MarvinBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::MaxHeapStrategy,  esacomp::MultimapBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::MaxLCPStrategy,   esacomp::MultimapBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::LazyListStrategy, esacomp::MultimapBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::MaxHeapStrategy,  esacomp::SuccinctListBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::MaxLCPStrategy,   esacomp::SuccinctListBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::LazyListStrategy, esacomp::SuccinctListBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::MaxHeapStrategy,  esacomp::DecodeForwardQueueListBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::MaxLCPStrategy,   esacomp::DecodeForwardQueueListBuffer>>();
    r.register_compressor<ESACompressor<Code2Coder, esacomp::LazyListStrategy, esacomp::DecodeForwardQueueListBuffer>>();

    r.register_compressor<ESACompressor<ASCIICoder, esacomp::MaxHeapStrategy,  esacomp::MarvinBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::MaxLCPStrategy,   esacomp::MarvinBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::LazyListStrategy, esacomp::MarvinBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::MaxHeapStrategy,  esacomp::MultimapBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::MaxLCPStrategy,   esacomp::MultimapBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::LazyListStrategy, esacomp::MultimapBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::MaxHeapStrategy,  esacomp::SuccinctListBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::MaxLCPStrategy,   esacomp::SuccinctListBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::LazyListStrategy, esacomp::SuccinctListBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::MaxHeapStrategy,  esacomp::DecodeForwardQueueListBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::MaxLCPStrategy,   esacomp::DecodeForwardQueueListBuffer>>();
    r.register_compressor<ESACompressor<ASCIICoder, esacomp::LazyListStrategy, esacomp::DecodeForwardQueueListBuffer>>();

    REGISTER_WITH_ONLINE_CODERS(LZSSSlidingWindowCompressor);
    REGISTER_WITH_ONLINE_CODERS(RunLengthEncoder);

    r.register_compressor<EasyRLECompressor>();
    r.register_compressor<MTFCompressor>();
    r.register_compressor<BWTCompressor>();

    r.register_compressor<ChainCompressor>();
    r.register_compressor<NoopCompressor>();
}

}

