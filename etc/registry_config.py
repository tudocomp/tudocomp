context_free_coder = [
    ("ASCIICoder",      "coders/ASCIICoder.hpp",      []),
    ("BitCoder",        "coders/BitCoder.hpp",        []),
    ("EliasGammaCoder", "coders/EliasGammaCoder.hpp", []),
    ("EliasDeltaCoder", "coders/EliasDeltaCoder.hpp", []),
]

# TODO: Fix bad interaction between sle and lz78u code and remove this distinction
tmp_lz78u_string_coder = context_free_coder + [
    ("HuffmanCoder", "coders/HuffmanCoder.hpp", []),
]

coder = tmp_lz78u_string_coder + [
    ("SLECoder",   "coders/SLECoder.hpp",   []),
]

hash_manager = [
        ("SizeManagerPow2", "util/Hash.hpp",  []),
        ("SizeManagerPrime", "util/Hash.hpp", []),
        ("SizeManagerDirect", "util/Hash.hpp", []),
        ]
hash_prober = [
        ("LinearProber", "util/Hash.hpp",  []),
        ]
hash_roll = [
        ("WordpackRollingHash", "util/Hash.hpp",  []),
        # ("ZBackupRollingHash", "util/Hash.hpp",  []),
        # ("CyclicHash", "util/hash/cyclichash.h",  []),
        ("KarpRabinHash", "util/hash/rabinkarphash.h",  []),
        # ("ThreeWiseHash", "util/hash/threewisehash.h",  []),
        ]
hash_function = [
        ("NoopHasher", "util/Hash.hpp",  []),
        ("MixHasher", "util/Hash.hpp",  []),
        ("VignaHasher", "util/Hash.hpp",  []),
        ("KnuthHasher", "util/Hash.hpp",  []),
        # ("Zobrist", "util/hash/zobrist.h",  []),
        # ("CLHash", "util/hash/clhash.h",  []),
        ]


lz78_trie = [
    ("lz78::BinarySortedTrie", "compressors/lz78/BinarySortedTrie.hpp", []),
    ("lz78::BinaryTrie",       "compressors/lz78/BinaryTrie.hpp",       []),
    ("lz78::CedarTrie",        "compressors/lz78/CedarTrie.hpp",        []),
    ("lz78::ExtHashTrie",       "compressors/lz78/ExtHashTrie.hpp",   []),
    ("lz78::HashTrie",         "compressors/lz78/HashTrie.hpp",         [hash_function,hash_prober,hash_manager]),
    ("lz78::HashTriePlus",         "compressors/lz78/HashTriePlus.hpp",         [hash_function,hash_manager]),
    ("lz78::RollingTrie",   "compressors/lz78/RollingTrie.hpp",   [hash_roll, hash_prober,hash_manager,hash_function]),
    ("lz78::RollingTriePlus",   "compressors/lz78/RollingTriePlus.hpp",   [hash_roll, hash_manager,hash_function]),
    ("lz78::TernaryTrie",      "compressors/lz78/TernaryTrie.hpp",      []),
]

if config_match("^#define JUDY_H_AVAILABLE 1"): lz78_trie += [
    ("lz78::JudyTrie",         "compressors/lz78/JudyTrie.hpp",         []),
]

lcpc_strat = [
    ("lcpcomp::MaxHeapStrategy",  "compressors/lcpcomp/compress/MaxHeapStrategy.hpp",   []),
    ("lcpcomp::MaxLCPStrategy",   "compressors/lcpcomp/compress/MaxLCPStrategy.hpp",    []),
    ("lcpcomp::ArraysComp", "compressors/lcpcomp/compress/ArraysComp.hpp",  []),
    ("lcpcomp::PLCPPeaksStrategy","compressors/lcpcomp/compress/PLCPPeaksStrategy.hpp", []),
]

if config_match("^#define Boost_FOUND 1"): lcpc_strat += [
    ("lcpcomp::BoostHeap",  "compressors/lcpcomp/compress/BoostHeap.hpp",   []),
    ("lcpcomp::PLCPStrategy",     "compressors/lcpcomp/compress/PLCPStrategy.hpp",      [])
    ]

lcpc_buffer = [
    ("lcpcomp::ScanDec",       "compressors/lcpcomp/decompress/ScanDec.hpp", []),
    ("lcpcomp::DecodeForwardQueueListBuffer", "compressors/lcpcomp/decompress/DecodeQueueListBuffer.hpp",  []),
    ("lcpcomp::CompactDec",           "compressors/lcpcomp/decompress/CompactDec.hpp",     []),
    ("lcpcomp::MultimapBuffer",               "compressors/lcpcomp/decompress/MultiMapBuffer.hpp",         []),
]

lcpc_coder = [
    ("ASCIICoder", "coders/ASCIICoder.hpp", []),
    ("SLECoder", "coders/SLECoder.hpp", []),
]

lz78u_strategy = [
    ("lz78u::StreamingStrategy", "compressors/lz78u/StreamingStrategy.hpp", [context_free_coder]),
    ("lz78u::BufferingStrategy", "compressors/lz78u/BufferingStrategy.hpp", [tmp_lz78u_string_coder]),
]

textds = [
    ("TextDS<>", "ds/TextDS.hpp", [])
]

# Top level types:

compressors = [
    ("LCPCompressor",               "compressors/LCPCompressor.hpp",               [lcpc_coder, lcpc_strat, lcpc_buffer, textds]),
    ("LZ78UCompressor",             "compressors/LZ78UCompressor.hpp",             [lz78u_strategy, context_free_coder]),
    ("RunLengthEncoder",            "compressors/RunLengthEncoder.hpp",            []),
    ("LiteralEncoder",              "compressors/LiteralEncoder.hpp",              [coder]),
    ("LZ78Compressor",              "compressors/LZ78Compressor.hpp",              [context_free_coder, lz78_trie]),
    ("LZWCompressor",               "compressors/LZWCompressor.hpp",               [context_free_coder, lz78_trie]),
    ("RePairCompressor",            "compressors/RePairCompressor.hpp",            [coder]),
    ("LZSSLCPCompressor",           "compressors/LZSSLCPCompressor.hpp",           [coder, textds]),
    ("LZSSSlidingWindowCompressor", "compressors/LZSSSlidingWindowCompressor.hpp", [context_free_coder]),
    ("MTFCompressor",               "compressors/MTFCompressor.hpp",               []),
    ("NoopCompressor",              "compressors/NoopCompressor.hpp",              []),
    ("BWTCompressor",               "compressors/BWTCompressor.hpp",               [textds]),
    ("ChainCompressor",             "../tudocomp_driver/ChainCompressor.hpp",      []),
]

generators = [
    ("FibonacciGenerator",     "generators/FibonacciGenerator.hpp", []),
    ("ThueMorseGenerator",     "generators/ThueMorseGenerator.hpp", []),
    ("RandomUniformGenerator", "generators/RandomUniformGenerator.hpp", []),
    ("RunRichGenerator",       "generators/RunRichGenerator.hpp", []),
]

# Invoke the code generation
generate_code([("Compressor", compressors), ("Generator", generators)])
