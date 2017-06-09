#!/usr/bin/python3

import re
import sys
import itertools

if not len(sys.argv[1:]) == 3:
    print(str.format("Usage {} [config.h] [selection] [outfile]", sys.argv[0]))
    sys.exit(1)

config_path = sys.argv[1]
selection = sys.argv[2]
outfile = sys.argv[3]

def config_match(pattern):
    textfile = open(config_path, 'r')
    filetext = textfile.read()
    textfile.close()
    pattern = re.compile(pattern)
    for line in filetext.splitlines():
        for match in re.finditer(pattern, line):
            return True
    return False

context_free_coder = [
    ("ASCIICoder",      "coders/ASCIICoder.hpp",      []),
    ("BitCoder",        "coders/BitCoder.hpp",        []),
    ("EliasGammaCoder", "coders/EliasGammaCoder.hpp", []),
    ("EliasDeltaCoder", "coders/EliasDeltaCoder.hpp", []),
    ("AdaptiveHuffmanCoder", "coders/AdaptiveHuffmanCoder.hpp", []),
]

# TODO: Fix bad interaction between sle and lz78u code and remove this distinction
tmp_lz78u_string_coder = context_free_coder + [
    ("HuffmanCoder", "coders/HuffmanCoder.hpp", []),
]

bit_interleaving_coder = [
    ("ArithmeticCoder", "coders/ArithmeticCoder.hpp", []),
]

coder = tmp_lz78u_string_coder + bit_interleaving_coder + [
    ("SLECoder",   "coders/SLECoder.hpp",   []),
] 

non_bit_interleaving_coder = [i for i in coder if i not in bit_interleaving_coder]

lz78_trie = [
    ("lz78::BinarySortedTrie", "compressors/lz78/BinarySortedTrie.hpp", []),
    ("lz78::BinaryTrie",       "compressors/lz78/BinaryTrie.hpp",       []),
    ("lz78::HashTrie",         "compressors/lz78/HashTrie.hpp",         []),
    ("lz78::MyHashTrie",       "compressors/lz78/MyHashTrie.hpp",       []),
    ("lz78::TernaryTrie",      "compressors/lz78/TernaryTrie.hpp",      []),
    ("lz78::CedarTrie",        "compressors/lz78/CedarTrie.hpp",        []),
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
    ("lcpcomp::MyMapBuffer",                  "compressors/lcpcomp/decompress/MyMapBuffer.hpp",            []),
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

compressors = [
    ("LCPCompressor",               "compressors/LCPCompressor.hpp",               [lcpc_coder, lcpc_strat, lcpc_buffer, textds]),
    ("LZ78UCompressor",             "compressors/LZ78UCompressor.hpp",             [lz78u_strategy, context_free_coder]),
    ("RunLengthEncoder",            "compressors/RunLengthEncoder.hpp",            []),
    ("LiteralEncoder",              "compressors/LiteralEncoder.hpp",              [coder]),
    ("LZ78Compressor",              "compressors/LZ78Compressor.hpp",              [context_free_coder, lz78_trie]),
    ("LZWCompressor",               "compressors/LZWCompressor.hpp",               [context_free_coder, lz78_trie]),
    ("RePairCompressor",            "compressors/RePairCompressor.hpp",            [non_bit_interleaving_coder]),
    ("LZSSLCPCompressor",           "compressors/LZSSLCPCompressor.hpp",           [non_bit_interleaving_coder, textds]),
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

algorithms_cpp_head = '''
/* Autogenerated file by genregistry.py */
'''

algorithms_cpp_decl = '''
#include <tudocomp_driver/Registry.hpp>

namespace tdc_algorithms {

using namespace tdc;

void register_compressors(Registry<Compressor>& r);
void register_generators(Registry<Generator>& r);

// One global instance for the registry
Registry<Compressor> COMPRESSOR_REGISTRY = Registry<Compressor>::with_all_from(register_compressors, "compressor");
Registry<Generator> GENERATOR_REGISTRY = Registry<Generator>::with_all_from(register_generators, "generator");

void register_compressors(Registry<Compressor>& r) {
$COMPRESSORS
}//function register_compressors

void register_generators(Registry<Generator>& r) {
$GENERATORS
}//function register_generators

}//ns
'''

tudocomp_hpp_template = '''
/*
    Autogenerated file by genregistry.py
    Include this to include practically all of tudocomp.

    This header also contains the Doxygen documentation of the main namespaces.
*/
'''

# Generates carthesian product of template params
def gen_list(ls):
    def expand_deps(algorithm):
        name = algorithm[0]
        deps = algorithm[2]

        deps_lists = [gen_list(x) for x in deps]
        deps_lists_prod = itertools.product(*deps_lists)

        return_list = []
        for deps_tuple in deps_lists_prod:
            if len(deps_tuple) == 0:
                return_list += [str.format("{}", name)]
            else:
                return_list += [str.format("{}<{}>", name, ",".join(deps_tuple))]

        assert len(return_list) != 0

        return return_list

    return_list = []
    for algorithm in ls:
        return_list += expand_deps(algorithm)
    return return_list

# Generates list of all includes
def gather_header(ls):
    headers = set()
    for e in ls:
        headers |= { e[1] }
        for a in e[2]:
            headers |= gather_header(a)
    return headers

# Output algorithm.cpp
def gen_algorithm_cpp():
    algorithms_cpp = algorithms_cpp_head
    for header in sorted(gather_header(compressors + generators)):
        algorithms_cpp += str.format("#include <tudocomp/{}>\n", header)

    algorithms_cpp += algorithms_cpp_decl
    l_compressors = []
    for line in gen_list(compressors):
        l_compressors += [str.format("    r.register_algorithm<{}>();", line)]

    l_generators = []
    for line in gen_list(generators):
        l_generators += [str.format("    r.register_algorithm<{}>();", line)]

    return algorithms_cpp.replace(
                "$COMPRESSORS", "\n".join(l_compressors)).replace(
                "$GENERATORS", "\n".join(l_generators))+ "\n"

if selection == "tudocomp_algorithms.cpp":
    file1 = open(outfile, 'w+')
    file1.write(gen_algorithm_cpp())
    file1.close()
else:
    sys.exit(1)
