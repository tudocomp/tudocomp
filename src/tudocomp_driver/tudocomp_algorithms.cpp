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
// - Display name: Used in the UI for dispalying the name of a algorthm
// - ID string: Used in the command line interface and the output filename
//   for identifying an algorithm
// - Description: Text describing what kind of algorithm this is.
// - Algorithm: A pointer to the base class of an Compressor or Encoder.

std::vector<CompressionAlgorithm> COMPRESSION_ALGORITHM = {
    {
        "Dummy",
        "dummy",
        "Dummy compressor that does not produce any rules.",
        new DummyCompressor()
    },
    {
        "LZ",
        "lz",
        "LZ impl included in esacomp.",
        new LZCompressor()
    },
    {
        "ESA",
        "esa",
        "Esacomp.",
        new ESACompressor<>()
    },
    {
        "ESA (Sorted Suffix List)",
        "esa_list",
        "Esacomp using a suffix list internally.",
        new ESACompressor<MaxLCPSortedSuffixList>()
    },
    {
        "ESA (Heap)",
        "esa_heap",
        "Esacomp using a heap internally.",
        new ESACompressor<MaxLCPHeap>()
    },
};

std::vector<CodingAlgorithm> CODING_ALGORITHM = {
    {
        "Dummy",
        "dummy",
        "Dummy encoding, outputs input text unchanged.",
        new DummyCoder()
    },
    {
        "Code0",
        "esa_code0",
        "Human readable output of text and rules.",
        new Code0Coder()
    },
    {
        "Code1",
        "esa_code1",
        "Byte based encoding.",
        new Code1Coder()
    },
    {
        "Code2",
        "esa_code2",
        "Bit based encoding.",
        new Code2Coder()
    },
};

}
