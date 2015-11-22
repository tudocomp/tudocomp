#include "tudocomp_algorithms.h"
#include "lz77rule.h"
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

using namespace tudocomp;
using namespace lz77rule;
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

void register_algos(AlgorithmRegistry<Compressor>& registry) {
    registry.with_info<Lz77Rule>(
        "LZ77 rule-like", "lz77rule",
        "A Family of compression algorithms making use "
        "of LZ77-like replacement rules.")
    .with_sub_algos<Lz77RuleCompressor>([](AlgorithmRegistry<Lz77RuleCompressor>& registry) {
        registry.set_name("Compressor");

        registry.with_info<DummyCompressor>(
            "Dummy", "dummy",
            "Dummy compressor that does not produce any rules.").do_register();

        registry.with_info<LZCompressor>(
            "LZ", "lz",
            "LZ impl included in esacomp.").do_register();

        registry.with_info<ESACompressor<>>(
            "ESA", "esa",
            "Esacomp (defaults to using a suffix list internally).").do_register();

        registry.with_info<ESACompressor<MaxLCPSortedSuffixList>>(
            "ESA (Sorted Suffix List)", "esa_list",
            "Esacomp using a suffix list internally.").do_register();

        registry.with_info<ESACompressor<MaxLCPHeap>>(
            "ESA (Heap)", "esa_heap",
            "Esacomp using a heap internally.").do_register();
    })
    .with_sub_algos<Lz77RuleCoder>([](AlgorithmRegistry<Lz77RuleCoder>& registry) {
        registry.set_name("Coder");

        registry.with_info<DummyCoder>(
            "Dummy", "dummy",
            "Dummy encoding, outputs input text unchanged.").do_register();

        registry.with_info<Code0Coder>(
            "Code0", "esa_code0",
            "Human readable output of text and rules.").do_register();

        registry.with_info<Code1Coder>(
            "Code1", "esa_code1",
            "Byte based encoding.").do_register();

        registry.with_info<Code2Coder>(
            "Code2", "esa_code2",
            "Bit based encoding.").do_register();
    })
    .do_register();
}

}
