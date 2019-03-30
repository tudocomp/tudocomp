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

##### lz trie #####

# LZ trie hash probers
hash_prober = [
        AlgorithmConfig(name="LinearProber", header="util/Hash.hpp"),
        ]
# LZ trie rolling hash functions
hash_roll = [
        AlgorithmConfig(name="WordpackRollingHash", header="util/Hash.hpp"),
        # AlgorithmConfig(name="ZBackupRollingHash", header="util/Hash.hpp"),
        # AlgorithmConfig(name="CyclicHash", header="util/hash/cyclichash.h"),
        AlgorithmConfig(name="rollinghash::KarpRabinHashAlgo", header="util/rollinghash/rabinkarphash.hpp"),
        # AlgorithmConfig(name="ThreeWiseHash", header="util/hash/threewisehash.h"),
        ]
# LZ trie hash functions
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
    AlgorithmConfig(name="lz_trie::ch::Sparse", header="compressors/lz_trie/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz_trie::ch::Plain", header="compressors/lz_trie/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz_trie::ch::SparseDisplacement", header="compressors/lz_trie/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz_trie::ch::PlainDisplacement", header="compressors/lz_trie/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz_trie::ch::SparseEliasDisplacement", header="compressors/lz_trie/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz_trie::ch::PlainEliasDisplacement", header="compressors/lz_trie/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz_trie::ch::SparseEliasGrowingDisplacement", header="compressors/lz_trie/CompactHashTrie.hpp"),
    AlgorithmConfig(name="lz_trie::ch::PlainEliasGrowingDisplacement", header="compressors/lz_trie/CompactHashTrie.hpp"),
]
compact_hashmap_strategies = compact_hashmap_strategies_norec + [
    AlgorithmConfig(name="lz_trie::ch::SplitKeyValue", header="compressors/lz_trie/CompactHashTrie.hpp", sub=[compact_hashmap_strategies_norec]),
    AlgorithmConfig(name="lz_trie::ch::SplitKey", header="compressors/lz_trie/CompactHashTrie.hpp", sub=[compact_hashmap_strategies_norec]),
]

# LZ tries ("lz_trie")
lz_trie = [
    AlgorithmConfig(name="lz_trie::BinarySortedTrie", header="compressors/lz_trie/BinarySortedTrie.hpp"),
    AlgorithmConfig(name="lz_trie::BinaryTrie", header="compressors/lz_trie/BinaryTrie.hpp"),
    AlgorithmConfig(name="lz_trie::CedarTrie", header="compressors/lz_trie/CedarTrie.hpp"),
    AlgorithmConfig(name="lz_trie::ExtHashTrie", header="compressors/lz_trie/ExtHashTrie.hpp"),
    AlgorithmConfig(name="lz_trie::HashTrie", header="compressors/lz_trie/HashTrie.hpp", sub=[hash_function,hash_prober]),
    AlgorithmConfig(name="lz_trie::HashTriePlus", header="compressors/lz_trie/HashTriePlus.hpp", sub=[hash_function]),
    AlgorithmConfig(name="lz_trie::RollingTrie", header="compressors/lz_trie/RollingTrie.hpp", sub=[hash_roll, hash_prober,hash_function]),
    AlgorithmConfig(name="lz_trie::RollingTriePlus", header="compressors/lz_trie/RollingTriePlus.hpp", sub=[hash_roll, hash_function]),
    AlgorithmConfig(name="lz_trie::TernaryTrie", header="compressors/lz_trie/TernaryTrie.hpp"),
    AlgorithmConfig(name="lz_trie::CompactHashTrie", header="compressors/lz_trie/CompactHashTrie.hpp", sub=[compact_hashmap_strategies]),
]

if config_match("^#define JUDY_H_AVAILABLE 1"): # if the Judy trie is available
    lz_trie += [
        AlgorithmConfig(name="lz_trie::JudyTrie", header="compressors/lz_trie/JudyTrie.hpp"),
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
    AlgorithmConfig(name="LZ78Compressor", header="compressors/LZ78Compressor.hpp", sub=[universal_coders, lz_trie]),
    AlgorithmConfig(name="LZWCompressor", header="compressors/LZWCompressor.hpp", sub=[universal_coders, lz_trie]),
    AlgorithmConfig(name="LZ78PointerJumpingCompressor", header="compressors/LZ78PointerJumpingCompressor.hpp", sub=[universal_coders, lz_trie]),
    AlgorithmConfig(name="LZWPointerJumpingCompressor", header="compressors/LZWPointerJumpingCompressor.hpp", sub=[universal_coders, lz_trie]),
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
