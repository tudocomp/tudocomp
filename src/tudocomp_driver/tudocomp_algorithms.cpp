#include "tudocomp_algorithms.h"
#include "lz77rule.h"
#include "lz78rule.h"
#include "lz_compressor.h"
#include "lz78.h"
#include "lz77.h"
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
using namespace esacomp;
using namespace dummy;
using namespace lz_compressor;

using lz78rule::Lz78Rule;

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
        // TODO: Name right
        // - lz77rule ~ LZSS rules with implicit escape bits
        // - lz ~ unknown named variant of lz with suffix array
        "LZ77 rule-like", "lz77rule",
        "A Family of compression algorithms making use "
        "of LZ77-like replacement rules.")
    .with_sub_algos<Lz77RuleCompressor>([](AlgorithmRegistry<Lz77RuleCompressor>& registry) {
        registry.set_name("Compressor");

        registry.with_info<DummyCompressor>(
            "Dummy", "dummy",
            "Dummy compressor that does not produce any rules.").do_register();

        registry.with_info<LZCompressor>(
            "LZ SA", "lz",
            "LZ impl included in esacomp. Uses a suffix array to determine the LZ factorisation up-front").do_register();

        registry.with_info<ESACompressor<>>(
            "ESA", "esa",
            "Esacomp (defaults to using a suffix list internally).").do_register();

        registry.with_info<ESACompressor<MaxLCPSortedSuffixList>>(
            "ESA (Sorted Suffix List)", "esa_list",
            "Esacomp using a suffix list internally.").do_register();

        registry.with_info<LZ77ClassicCompressor>(
            "LZ77 Classic", "lz77",
            "LZ77, using a fixed sized dictionary and preview window").do_register();

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

    registry.with_info<Lz78Rule>(
        "LZ78 rule-like", "lz78rule",
        "A Family of compression algorithms making use "
        "of LZ78-like replacement rules.")
    .with_sub_algos<Lz78RuleCompressor>([](AlgorithmRegistry<Lz78RuleCompressor>& registry) {
        registry.set_name("Compressor");

        registry.with_info<LZ78Compressor>(
            "lz78", "lz78",
            "Lz78 compressor that has a unlimited dictionary").do_register();
    })
    .with_sub_algos<Lz78RuleCoder>([](AlgorithmRegistry<Lz78RuleCoder>& registry) {
        registry.set_name("Coder");

        registry.with_info<LZ78DebugCode>(
            "Debug", "debug",
            "Debug encoding, each rule is emitted as a string of the form `(<idx>,<chr>)`").do_register();

        registry.with_info<LZ78BitCode>(
            "Bit", "bit",
            "Bit encoding, each rule is emitted a bitstream of the "
            "minimum amount of bits needed to encode the index and the char").do_register();
    })
    .do_register();
}

}
