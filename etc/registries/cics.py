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
    # AlgorithmConfig(name="BinaryCoder", header="coders/BinaryCoder.hpp"),
    # AlgorithmConfig(name="EliasGammaCoder", header="coders/EliasGammaCoder.hpp"),
    # AlgorithmConfig(name="EliasDeltaCoder", header="coders/EliasDeltaCoder.hpp"),
    # AlgorithmConfig(name="SLEIntCoder", header="coders/SLEIntCoder.hpp"),
]

# Entropy coders
entropy_coders = [
    # AlgorithmConfig(name="SigmaCoder", header="coders/SigmaCoder.hpp"),
    # AlgorithmConfig(name="HuffmanCoder", header="coders/HuffmanCoder.hpp"),
]

# Entropy coders that may consume characters without immediately generating an
# output (not suitable for all scenarios, see "Interleaved Coding" section in
# the documentation)
consuming_entropy_coders = [
    # AlgorithmConfig(name="ArithmeticCoder", header="coders/ArithmeticCoder.hpp"),
    # AlgorithmConfig(name="SLEKmerCoder", header="coders/SLEKmerCoder.hpp"),
]

# All non-consuming coders
non_consuming_coders = universal_coders + entropy_coders

# All coders
all_coders = universal_coders + entropy_coders + consuming_entropy_coders

##### LZSS Coding Strategies #####
lzss_streaming_coders = [
    # AlgorithmConfig(name="lzss::DidacticalCoder", header="compressors/lzss/DidacticalCoder.hpp"),
    AlgorithmConfig(name="lzss::StreamingCoder", header="compressors/lzss/StreamingCoder.hpp",
        sub=[universal_coders,universal_coders,universal_coders]),
]

lzss_coders = lzss_streaming_coders + [
    AlgorithmConfig(name="lzss::BufferedLeftCoder", header="compressors/lzss/BufferedLeftCoder.hpp",
        sub=[universal_coders,universal_coders,all_coders]),
    AlgorithmConfig(name="lzss::BufferedBidirectionalCoder", header="compressors/lzss/BufferedBidirectionalCoder.hpp",
        sub=[universal_coders,universal_coders,all_coders]),
]


##### Export available compressors #####
tdc.compressors = [
    AlgorithmConfig(name="LZ78CicsCompressor", header="compressors/LZ78CicsCompressor.hpp", sub=[universal_coders]),
    AlgorithmConfig(name="LZSSCicsCompressor", header="compressors/LZSSCicsCompressor.hpp", sub=[lzss_streaming_coders]),
    AlgorithmConfig(name="NoopCompressor", header="compressors/NoopCompressor.hpp"),
]

##### Export available string generators #####
tdc.generators = [
    AlgorithmConfig(name="FibonacciGenerator", header="generators/FibonacciGenerator.hpp"),
    AlgorithmConfig(name="ThueMorseGenerator", header="generators/ThueMorseGenerator.hpp"),
    AlgorithmConfig(name="RandomUniformGenerator", header="generators/RandomUniformGenerator.hpp"),
    AlgorithmConfig(name="RunRichGenerator", header="generators/RunRichGenerator.hpp"),
]
