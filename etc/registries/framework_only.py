###############################################################################
#
# This script determines which algorithms will be available in the tdc binary.
#
# Please refer to the documentation ("Registering Algorithms" section) for
# more information.
#
###############################################################################

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
