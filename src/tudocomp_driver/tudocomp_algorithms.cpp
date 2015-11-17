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

REGISTER_ALGO(LZ77_RULE_COMP_ALGOS, Lz77RuleCompressor,
              DummyCompressor, "Dummy", "dummy",
              "Dummy compressor that does not produce any rules.")

REGISTER_ALGO(LZ77_RULE_COMP_ALGOS, Lz77RuleCompressor,
              LZCompressor, "LZ", "lz",
              "LZ impl included in esacomp.")

using ESACompressorDefault = ESACompressor<>;
REGISTER_ALGO(LZ77_RULE_COMP_ALGOS, Lz77RuleCompressor,
              ESACompressorDefault, "ESA", "esa",
              "Esacomp (defaults to using a suffix list internally).")

using ESACompressorList = ESACompressor<MaxLCPSortedSuffixList>;
REGISTER_ALGO(LZ77_RULE_COMP_ALGOS, Lz77RuleCompressor,
              ESACompressorList, "ESA (Sorted Suffix List)", "esa_list",
              "Esacomp using a suffix list internally.")

using ESACompressorHeap = ESACompressor<MaxLCPHeap>;
REGISTER_ALGO(LZ77_RULE_COMP_ALGOS, Lz77RuleCompressor,
              ESACompressorHeap, "ESA (Heap)", "esa_heap",
              "Esacomp using a heap internally.")

/////////////////////////////////////////////////////////////////////////////

REGISTER_ALGO(LZ77_RULE_CODE_ALGOS, Lz77RuleCoder,
              DummyCoder, "Dummy", "dummy",
              "Dummy encoding, outputs input text unchanged.")

REGISTER_ALGO(LZ77_RULE_CODE_ALGOS, Lz77RuleCoder,
              Code0Coder, "Code0", "esa_code0",
              "Human readable output of text and rules.")

REGISTER_ALGO(LZ77_RULE_CODE_ALGOS, Lz77RuleCoder,
              Code1Coder, "Code1", "esa_code1",
              "Byte based encoding.")

REGISTER_ALGO(LZ77_RULE_CODE_ALGOS, Lz77RuleCoder,
              Code2Coder, "Code2", "esa_code2",
              "Bit based encoding.")

}
