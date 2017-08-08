context_free_coder = [
    AlgorithmConfig(name="ASCIICoder", header="coders/ASCIICoder.hpp"),
    AlgorithmConfig(name="BitCoder", header="coders/BitCoder.hpp"),
    AlgorithmConfig(name="EliasGammaCoder", header="coders/EliasGammaCoder.hpp"),
    AlgorithmConfig(name="EliasDeltaCoder", header="coders/EliasDeltaCoder.hpp"),
]

# TODO: Fix bad interaction between sle and lz78u code and remove this distinction
tmp_lz78u_string_coder = context_free_coder + [
    AlgorithmConfig(name="HuffmanCoder", header="coders/HuffmanCoder.hpp"),
]

bit_interleaving_coder = [
    AlgorithmConfig(name="ArithmeticCoder", header="coders/ArithmeticCoder.hpp"),
]

coder = tmp_lz78u_string_coder + bit_interleaving_coder + [
    AlgorithmConfig(name="SLECoder", header="coders/SLECoder.hpp"),
]

non_bit_interleaving_coder = [i for i in coder if i not in bit_interleaving_coder]

hash_manager = [
        AlgorithmConfig(name="SizeManagerPow2", header="util/Hash.hpp"),
        AlgorithmConfig(name="SizeManagerPrime", header="util/Hash.hpp"),
        AlgorithmConfig(name="SizeManagerDirect", header="util/Hash.hpp"),
        ]
hash_prober = [
        AlgorithmConfig(name="LinearProber", header="util/Hash.hpp"),
        ]
hash_roll = [
        AlgorithmConfig(name="WordpackRollingHash", header="util/Hash.hpp"),
        # AlgorithmConfig(name="ZBackupRollingHash", header="util/Hash.hpp"),
        # AlgorithmConfig(name="CyclicHash", header="util/hash/cyclichash.h"),
        AlgorithmConfig(name="KarpRabinHash", header="util/hash/rabinkarphash.h"),
        # AlgorithmConfig(name="ThreeWiseHash", header="util/hash/threewisehash.h"),
        ]
hash_function = [
        AlgorithmConfig(name="NoopHasher", header="util/Hash.hpp"),
        AlgorithmConfig(name="MixHasher", header="util/Hash.hpp"),
        AlgorithmConfig(name="VignaHasher", header="util/Hash.hpp"),
        AlgorithmConfig(name="KnuthHasher", header="util/Hash.hpp"),
        # AlgorithmConfig(name="Zobrist", header="util/hash/zobrist.h"),
        # AlgorithmConfig(name="CLHash", header="util/hash/clhash.h"),
        ]


lz78_trie = [
    AlgorithmConfig(name="lz78::BinarySortedTrie", header="compressors/lz78/BinarySortedTrie.hpp"),
    AlgorithmConfig(name="lz78::BinaryTrie", header="compressors/lz78/BinaryTrie.hpp"),
    AlgorithmConfig(name="lz78::CedarTrie", header="compressors/lz78/CedarTrie.hpp"),
    AlgorithmConfig(name="lz78::ExtHashTrie", header="compressors/lz78/ExtHashTrie.hpp"),
    AlgorithmConfig(name="lz78::HashTrie", header="compressors/lz78/HashTrie.hpp", sub=[hash_function,hash_prober,hash_manager]),
    AlgorithmConfig(name="lz78::HashTriePlus", header="compressors/lz78/HashTriePlus.hpp", sub=[hash_function,hash_manager]),
    AlgorithmConfig(name="lz78::RollingTrie", header="compressors/lz78/RollingTrie.hpp", sub=[hash_roll, hash_prober,hash_manager,hash_function]),
    AlgorithmConfig(name="lz78::RollingTriePlus", header="compressors/lz78/RollingTriePlus.hpp", sub=[hash_roll, hash_manager,hash_function]),
    AlgorithmConfig(name="lz78::TernaryTrie", header="compressors/lz78/TernaryTrie.hpp"),
]

if config_match("^#define JUDY_H_AVAILABLE 1"): lz78_trie += [
    AlgorithmConfig(name="lz78::JudyTrie", header="compressors/lz78/JudyTrie.hpp"),
]

lcpc_strat = [
    AlgorithmConfig(name="lcpcomp::MaxHeapStrategy", header="compressors/lcpcomp/compress/MaxHeapStrategy.hpp"),
    AlgorithmConfig(name="lcpcomp::MaxLCPStrategy", header="compressors/lcpcomp/compress/MaxLCPStrategy.hpp"),
    AlgorithmConfig(name="lcpcomp::ArraysComp", header="compressors/lcpcomp/compress/ArraysComp.hpp"),
    AlgorithmConfig(name="lcpcomp::PLCPPeaksStrategy", header="compressors/lcpcomp/compress/PLCPPeaksStrategy.hpp"),
]

if config_match("^#define Boost_FOUND 1"): lcpc_strat += [
    AlgorithmConfig(name="lcpcomp::BoostHeap", header="compressors/lcpcomp/compress/BoostHeap.hpp"),
    AlgorithmConfig("lcpcomp::PLCPStrategy", header="compressors/lcpcomp/compress/PLCPStrategy.hpp")
    ]

lcpc_buffer = [
    AlgorithmConfig(name="lcpcomp::ScanDec", header="compressors/lcpcomp/decompress/ScanDec.hpp"),
    AlgorithmConfig(name="lcpcomp::DecodeForwardQueueListBuffer", header="compressors/lcpcomp/decompress/DecodeQueueListBuffer.hpp"),
    AlgorithmConfig(name="lcpcomp::CompactDec", header="compressors/lcpcomp/decompress/CompactDec.hpp"),
    AlgorithmConfig(name="lcpcomp::MultimapBuffer", header="compressors/lcpcomp/decompress/MultiMapBuffer.hpp"),
]

lcpc_coder = [
    AlgorithmConfig(name="ASCIICoder", header="coders/ASCIICoder.hpp"),
    AlgorithmConfig(name="SLECoder", header="coders/SLECoder.hpp"),
    AlgorithmConfig(name="HuffmanCoder", header="coders/HuffmanCoder.hpp"),
]

lz78u_strategy = [
    AlgorithmConfig(name="lz78u::StreamingStrategy", header="compressors/lz78u/StreamingStrategy.hpp", sub=[context_free_coder]),
    AlgorithmConfig(name="lz78u::BufferingStrategy", header="compressors/lz78u/BufferingStrategy.hpp", sub=[tmp_lz78u_string_coder]),
]

textds = [
    AlgorithmConfig(name="TextDS<>", header="ds/TextDS.hpp")
]

# Export compressors and generators
tdc.compressors = [
    AlgorithmConfig(name="LCPCompressor", header="compressors/LCPCompressor.hpp", sub=[lcpc_coder, lcpc_strat, lcpc_buffer, textds]),
    AlgorithmConfig(name="LZ78UCompressor", header="compressors/LZ78UCompressor.hpp", sub=[lz78u_strategy, context_free_coder]),
    AlgorithmConfig(name="RunLengthEncoder", header="compressors/RunLengthEncoder.hpp"),
    AlgorithmConfig(name="LiteralEncoder", header="compressors/LiteralEncoder.hpp", sub=[coder]),
    AlgorithmConfig(name="LZ78Compressor", header="compressors/LZ78Compressor.hpp", sub=[context_free_coder, lz78_trie]),
    AlgorithmConfig(name="LZWCompressor", header="compressors/LZWCompressor.hpp", sub=[context_free_coder, lz78_trie]),
    AlgorithmConfig(name="RePairCompressor", header="compressors/RePairCompressor.hpp", sub=[non_bit_interleaving_coder]),
    AlgorithmConfig(name="LZSSLCPCompressor", header="compressors/LZSSLCPCompressor.hpp", sub=[non_bit_interleaving_coder, textds]),
    AlgorithmConfig(name="LZSSSlidingWindowCompressor", header="compressors/LZSSSlidingWindowCompressor.hpp", sub=[context_free_coder]),
    AlgorithmConfig(name="MTFCompressor", header="compressors/MTFCompressor.hpp"),
    AlgorithmConfig(name="NoopCompressor", header="compressors/NoopCompressor.hpp"),
    AlgorithmConfig(name="BWTCompressor", header="compressors/BWTCompressor.hpp", sub=[textds]),
    AlgorithmConfig(name="ChainCompressor", header="../tudocomp_driver/ChainCompressor.hpp"),
]

tdc.generators = [
    AlgorithmConfig(name="FibonacciGenerator", header="generators/FibonacciGenerator.hpp"),
    AlgorithmConfig(name="ThueMorseGenerator", header="generators/ThueMorseGenerator.hpp"),
    AlgorithmConfig(name="RandomUniformGenerator", header="generators/RandomUniformGenerator.hpp"),
    AlgorithmConfig(name="RunRichGenerator", header="generators/RunRichGenerator.hpp"),
]

