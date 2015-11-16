#include "tudocomp_algorithms.h"

#include "lz_compressor.h"
#include "esa_compressor.h"
#include "max_lcp_sorted_suffix_list.h"
#include "max_lcp_heap.h"
#include "dummy_compressor.h"

#include "code0.h"
#include "code1.h"
#include "code2.h"
#include "dummy_encoder.h"

namespace tudocomp_driver {

using namespace esacomp;
using namespace dummy;
using namespace lz_compressor;

// All compression and encoding algorithms exposed by the command
// line interface.
//
// The format of the initializers is a tuple of three things:
// - Display name: Used in the UI for displaying the name of a algorithm
// - ID string: Used in the command line interface and the output file name
//   for identifying an algorithm
// - Description: Text describing what kind of algorithm this is.
// - Algorithm: A function pointer that constructs the base class of a
//   Compressor or Encoder.

template<class T> Lz77RuleCompressor* compressor(Env& env) {
    return new T(env);
}

//REGISTER_ALGO(TUDOCOMP_ALGOS, DummyCompressor)

std::vector<CompressionAlgorithm> COMPRESSION_ALGORITHM = {
    {
        "Dummy",
        "dummy",
        "Dummy compressor that does not produce any rules.",
        &tudocomp::construct<Lz77RuleCompressor, DummyCompressor, Env&>
    },
    {
        "LZ",
        "lz",
        "LZ impl included in esacomp.",
        &compressor<LZCompressor>,
    },
    {
        "ESA",
        "esa",
        "Esacomp (defaults to using a suffix list internally).",
        &compressor<ESACompressor<>>,
    },
    {
        "ESA (Sorted Suffix List)",
        "esa_list",
        "Esacomp using a suffix list internally.",
        &compressor<ESACompressor<MaxLCPSortedSuffixList>>,
    },
    {
        "ESA (Heap)",
        "esa_heap",
        "Esacomp using a heap internally.",
        &compressor<ESACompressor<MaxLCPHeap>>,
    },
};

template<class T> Lz77RuleCoder* coder(Env& env) {
    return new T(env);
}

std::vector<CodingAlgorithm> CODING_ALGORITHM = {
    {
        "Dummy",
        "dummy",
        "Dummy encoding, outputs input text unchanged.",
        &coder<DummyCoder>,
    },
    {
        "Code0",
        "esa_code0",
        "Human readable output of text and rules.",
        &coder<Code0Coder>,
    },
    {
        "Code1",
        "esa_code1",
        "Byte based encoding.",
        &coder<Code1Coder>,
    },
    {
        "Code2",
        "esa_code2",
        "Bit based encoding.",
        &coder<Code2Coder>,
    },
};

}
