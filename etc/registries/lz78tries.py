###############################################################################
#
# This script determines which algorithms will be available in the tdc binary.
#
# Please refer to the documentation ("Registering Algorithms" section) for
# more information.
#
###############################################################################

##### Coders #####

# Universal coders
universal_coders = [
    AlgorithmConfig(name="ASCIICoder", header="coders/ASCIICoder.hpp"),
    AlgorithmConfig(name="BinaryCoder", header="coders/BinaryCoder.hpp"),
    AlgorithmConfig(name="EliasGammaCoder", header="coders/EliasGammaCoder.hpp"),
    AlgorithmConfig(name="EliasDeltaCoder", header="coders/EliasDeltaCoder.hpp"),
    AlgorithmConfig(name="SLEIntCoder", header="coders/SLEIntCoder.hpp"),
]

# Entropy coders
entropy_coders = [
    AlgorithmConfig(name="SigmaCoder", header="coders/SigmaCoder.hpp"),
    AlgorithmConfig(name="HuffmanCoder", header="coders/HuffmanCoder.hpp"),
]

# Entropy coders that may consume characters without immediately generating an
# output (not suitable for all scenarios, see "Interleaved Coding" section in
# the documentation)
consuming_entropy_coders = [
    AlgorithmConfig(name="ArithmeticCoder", header="coders/ArithmeticCoder.hpp"),
    AlgorithmConfig(name="SLEKmerCoder", header="coders/SLEKmerCoder.hpp"),
]

# All non-consuming coders
non_consuming_coders = universal_coders + entropy_coders

# All coders
all_coders = universal_coders + entropy_coders + consuming_entropy_coders

##### lz78 #####

# LZ78 trie hash probers
hash_prober = [
        AlgorithmConfig(name="LinearProber", header="util/Hash.hpp"),
        ]
# LZ78 trie rolling hash functions
hash_roll = [
        AlgorithmConfig(name="WordpackRollingHash", header="util/Hash.hpp"),
        # AlgorithmConfig(name="ZBackupRollingHash", header="util/Hash.hpp"),
        # AlgorithmConfig(name="CyclicHash", header="util/hash/cyclichash.h"),
        AlgorithmConfig(name="KarpRabinHash", header="util/hash/rabinkarphash.h"),
        # AlgorithmConfig(name="ThreeWiseHash", header="util/hash/threewisehash.h"),
        ]
# LZ78 trie hash functions
hash_function = [
        AlgorithmConfig(name="NoopHasher", header="util/Hash.hpp"),
        AlgorithmConfig(name="MixHasher", header="util/Hash.hpp"),
        AlgorithmConfig(name="VignaHasher", header="util/Hash.hpp"),
        AlgorithmConfig(name="KnuthHasher", header="util/Hash.hpp"),
        # AlgorithmConfig(name="Zobrist", header="util/hash/zobrist.h"),
        # AlgorithmConfig(name="CLHash", header="util/hash/clhash.h"),
        ]

# CompactHashTrie strategies
compact_hashmap_strategies_norec = [
    AlgorithmConfig(name="lz78::ch::Sparse", header="compressors/lz78/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz78::ch::Plain", header="compressors/lz78/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz78::ch::SparseDisplacement", header="compressors/lz78/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz78::ch::PlainDisplacement", header="compressors/lz78/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz78::ch::SparseEliasDisplacement", header="compressors/lz78/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz78::ch::PlainEliasDisplacement", header="compressors/lz78/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz78::ch::SparseEliasGrowingDisplacement", header="compressors/lz78/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz78::ch::PlainEliasGrowingDisplacement", header="compressors/lz78/CompactHashTrie.hpp"),
]
compact_hashmap_strategies = compact_hashmap_strategies_norec + [
    AlgorithmConfig(name="lz78::ch::NoKVGrow", header="compressors/lz78/CompactHashTrie.hpp", sub=[compact_hashmap_strategies_norec]),
    AlgorithmConfig(name="lz78::ch::NoKGrow", header="compressors/lz78/CompactHashTrie.hpp", sub=[compact_hashmap_strategies_norec]),
]

# LZ78 tries ("lz78trie")
lz78_trie = [
    AlgorithmConfig(name="lz78::BinarySortedTrie", header="compressors/lz78/BinarySortedTrie.hpp"),
    AlgorithmConfig(name="lz78::BinaryTrie", header="compressors/lz78/BinaryTrie.hpp"),
    AlgorithmConfig(name="lz78::CedarTrie", header="compressors/lz78/CedarTrie.hpp"),
    AlgorithmConfig(name="lz78::ExtHashTrie", header="compressors/lz78/ExtHashTrie.hpp"),
    AlgorithmConfig(name="lz78::HashTrie", header="compressors/lz78/HashTrie.hpp", sub=[hash_function,hash_prober]),
    AlgorithmConfig(name="lz78::HashTriePlus", header="compressors/lz78/HashTriePlus.hpp", sub=[hash_function]),
    AlgorithmConfig(name="lz78::RollingTrie", header="compressors/lz78/RollingTrie.hpp", sub=[hash_roll, hash_prober,hash_function]),
    AlgorithmConfig(name="lz78::RollingTriePlus", header="compressors/lz78/RollingTriePlus.hpp", sub=[hash_roll, hash_function]),
    AlgorithmConfig(name="lz78::TernaryTrie", header="compressors/lz78/TernaryTrie.hpp"),
    AlgorithmConfig(name="lz78::CompactHashTrie", header="compressors/lz78/CompactHashTrie.hpp", sub=[compact_hashmap_strategies]),
]

if config_match("^#define JUDY_H_AVAILABLE 1"): # if the Judy trie is available
    lz78_trie += [
        AlgorithmConfig(name="lz78::JudyTrie", header="compressors/lz78/JudyTrie.hpp"),
]

lit_coder = [
    AlgorithmConfig(name="BinaryCoder", header="coders/BinaryCoder.hpp"),
    AlgorithmConfig(name="ASCIICoder", header="coders/ASCIICoder.hpp"),
    AlgorithmConfig(name="HuffmanCoder", header="coders/HuffmanCoder.hpp"),
]

len_coder = universal_coders

dividing_strat = [
    AlgorithmConfig(name="DivisionDividingStrategy", header="compressors/DividingCompressor.hpp"),
    AlgorithmConfig(name="BlockedDividingStrategy", header="compressors/DividingCompressor.hpp"),
]

long_common_strat = [
    AlgorithmConfig(name="EscapingSparseFactorCoder", header="compressors/LongCommonStringCompressor.hpp"),
]

##### Export available compressors #####
tdc.compressors = [
    AlgorithmConfig(name="RunLengthEncoder", header="compressors/RunLengthEncoder.hpp"),
    AlgorithmConfig(name="LiteralEncoder", header="compressors/LiteralEncoder.hpp", sub=[all_coders]),
    AlgorithmConfig(name="LZ78Compressor", header="compressors/LZ78Compressor.hpp", sub=[universal_coders, lz78_trie]),
    AlgorithmConfig(name="LZWCompressor", header="compressors/LZWCompressor.hpp", sub=[universal_coders, lz78_trie]),
    AlgorithmConfig(name="NoopCompressor", header="compressors/NoopCompressor.hpp"),
    AlgorithmConfig(name="ChainCompressor", header="compressors/ChainCompressor.hpp"),
    AlgorithmConfig(name="DividingCompressor", header="compressors/DividingCompressor.hpp", sub=[dividing_strat]),
    AlgorithmConfig(name="LongCommonStringCompressor", header="compressors/LongCommonStringCompressor.hpp", sub=[long_common_strat]),
]

##### Export available decompressors #####
tdc.decompressors = [
    AlgorithmConfig(name="LZ78Decompressor", header="decompressors/LZ78Decompressor.hpp", sub=[universal_coders]),
    AlgorithmConfig(name="LZWDecompressor", header="decompressors/LZWDecompressor.hpp", sub=[universal_coders]),
    AlgorithmConfig(name="WrapDecompressor", header="decompressors/WrapDecompressor.hpp"),
]

##### Export available string generators #####
tdc.generators = [
    AlgorithmConfig(name="FibonacciGenerator", header="generators/FibonacciGenerator.hpp"),
    AlgorithmConfig(name="ThueMorseGenerator", header="generators/ThueMorseGenerator.hpp"),
    AlgorithmConfig(name="RandomUniformGenerator", header="generators/RandomUniformGenerator.hpp"),
    AlgorithmConfig(name="RunRichGenerator", header="generators/RunRichGenerator.hpp"),
]
