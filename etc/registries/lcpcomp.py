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
    AlgorithmConfig(name="RiceCoder", header="coders/RiceCoder.hpp"),
]

# Entropy coders
entropy_coders = [
    AlgorithmConfig(name="HuffmanCoder", header="coders/HuffmanCoder.hpp"),
]

# Entropy coders that may consume characters without immediately generating an
# output (not suitable for all scenarios, see "Interleaved Coding" section in
# the documentation)
consuming_entropy_coders = [
    AlgorithmConfig(name="SLEKmerCoder", header="coders/SLEKmerCoder.hpp"),
]

# All non-consuming coders
non_consuming_coders = universal_coders + entropy_coders

# All coders
all_coders = universal_coders + entropy_coders + consuming_entropy_coders

##### LZSS Coding Strategies #####
lzss_streaming_coders = [
    AlgorithmConfig(name="lzss::DidacticalCoder", header="compressors/lzss/DidacticalCoder.hpp"),
    AlgorithmConfig(name="lzss::StreamingCoder", header="compressors/lzss/StreamingCoder.hpp",
        sub=[universal_coders,universal_coders,universal_coders]),
]

lzss_coders = lzss_streaming_coders + [
    AlgorithmConfig(name="lzss::BufferedLeftCoder", header="compressors/lzss/BufferedLeftCoder.hpp",
        sub=[universal_coders,universal_coders,all_coders]),
    AlgorithmConfig(name="lzss::BufferedBidirectionalCoder", header="compressors/lzss/BufferedBidirectionalCoder.hpp",
        sub=[universal_coders,universal_coders,all_coders]),
]

##### Text data structures #####

# Suffix Array
sa = [
    AlgorithmConfig(name="DivSufSort", header="ds/providers/DivSufSort.hpp"),
]

# Phi Array
phi = [
    AlgorithmConfig(name="PhiFromSA", header="ds/providers/PhiFromSA.hpp"),
]

# PLCP Array
plcp = [
    AlgorithmConfig(name="PhiAlgorithm", header="ds/providers/PhiAlgorithm.hpp"),
]

# Uncompressed LCP Array
lcp_uncompressed = [
    AlgorithmConfig(name="LCPFromPLCP", header="ds/providers/LCPFromPLCP.hpp"),
]

# All LCP Arrays
lcp = lcp_uncompressed + [
    #AlgorithmConfig(name="CompressedLCP", header="ds/CompressedLCP.hpp", sub=[sa]),
]

# Inverse Suffix Array
isa = [
    AlgorithmConfig(name="ISAFromSA", header="ds/providers/ISAFromSA.hpp"),
    AlgorithmConfig(name="SparseISA", header="ds/providers/SparseISA.hpp", sub=[sa]),
]

# TextDS
textds = [
    AlgorithmConfig(name="DSManager", header="ds/DSManager.hpp", sub=[sa, phi, plcp, lcp, isa]),
]

textds_lcp = [
    AlgorithmConfig(name="DSManager", header="ds/DSManager.hpp", sub=[sa, phi, plcp, lcp, isa]),
]

textds_lcpcomp = [
    AlgorithmConfig(name="DSManager", header="ds/DSManager.hpp", sub=[sa, phi, plcp, lcp_uncompressed, isa]),
]

##### lcpcomp #####

# lcpcomp factorization strategies ("comp")
lcpcomp_comp = [
    AlgorithmConfig(name="lcpcomp::MaxHeapStrategy", header="compressors/lcpcomp/compress/MaxHeapStrategy.hpp"),
    AlgorithmConfig(name="lcpcomp::MaxLCPStrategy", header="compressors/lcpcomp/compress/MaxLCPStrategy.hpp"),
    AlgorithmConfig(name="lcpcomp::ArraysComp", header="compressors/lcpcomp/compress/ArraysComp.hpp"),
]

if config_match("^#define SDSL_FOUND 1"): # if SDSL is available
    lcpcomp_comp += [
        AlgorithmConfig(name="lcpcomp::PLCPStrategy", header="compressors/lcpcomp/compress/PLCPStrategy.hpp"),
        AlgorithmConfig(name="lcpcomp::PLCPPeaksStrategy", header="compressors/lcpcomp/compress/PLCPPeaksStrategy.hpp"),
    ]

# lcpcomp factor decoding strategies ("dec")
lcpcomp_dec = [
    AlgorithmConfig(name="lcpcomp::PointerJumpIntEM", header="compressors/lcpcomp/decompress/PointerJumpIntEM.hpp"),
    AlgorithmConfig(name="lcpcomp::DecodeForwardQueueListBuffer", header="compressors/lcpcomp/decompress/DecodeQueueListBuffer.hpp"),
    AlgorithmConfig(name="lcpcomp::CompactDec", header="compressors/lcpcomp/decompress/CompactDec.hpp"),
    AlgorithmConfig(name="lcpcomp::MultimapBuffer", header="compressors/lcpcomp/decompress/MultiMapBuffer.hpp"),
]

if config_match("^#define SDSL_FOUND 1"): # if SDSL is available
    lcpcomp_dec += [
        AlgorithmConfig(name="lcpcomp::ScanDec", header="compressors/lcpcomp/decompress/ScanDec.hpp"),
        AlgorithmConfig(name="lcpcomp::PointerJump", header="compressors/lcpcomp/decompress/PointerJump.hpp"),
    ]

##### Misc #####

dividing_strat = [
    AlgorithmConfig(name="DivisionDividingStrategy", header="compressors/DividingCompressor.hpp"),
    AlgorithmConfig(name="BlockedDividingStrategy", header="compressors/DividingCompressor.hpp"),
]

long_common_strat = [
    AlgorithmConfig(name="EscapingSparseFactorCoder", header="compressors/LongCommonStringCompressor.hpp"),
]

##### Export available compressors #####
tdc.compressors = [
    AlgorithmConfig(name="LCPCompressor", header="compressors/LCPCompressor.hpp", sub=[lzss_bidirectional_coders, lcpcomp_comp, textds_lcpcomp]),
    AlgorithmConfig(name="LiteralEncoder", header="compressors/LiteralEncoder.hpp", sub=[all_coders]),
    AlgorithmConfig(name="LZSSLCPCompressor", header="compressors/LZSSLCPCompressor.hpp", sub=[lzss_coders, textds_lcp]),
    AlgorithmConfig(name="LZSSSlidingWindowCompressor", header="compressors/LZSSSlidingWindowCompressor.hpp", sub=[lzss_streaming_coders]),
    AlgorithmConfig(name="NoopCompressor", header="compressors/NoopCompressor.hpp"),
    AlgorithmConfig(name="ChainCompressor", header="compressors/ChainCompressor.hpp"),
    AlgorithmConfig(name="DividingCompressor", header="compressors/DividingCompressor.hpp", sub=[dividing_strat]),
    AlgorithmConfig(name="LongCommonStringCompressor", header="compressors/LongCommonStringCompressor.hpp", sub=[long_common_strat]),
]

##### Export available string generators #####
tdc.generators = [
    AlgorithmConfig(name="FibonacciGenerator", header="generators/FibonacciGenerator.hpp"),
    AlgorithmConfig(name="ThueMorseGenerator", header="generators/ThueMorseGenerator.hpp"),
    AlgorithmConfig(name="RandomUniformGenerator", header="generators/RandomUniformGenerator.hpp"),
    AlgorithmConfig(name="RunRichGenerator", header="generators/RunRichGenerator.hpp"),
]
