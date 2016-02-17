#include "tudocomp_algorithms.h"
#include "esacomp/esacomp_rule_compressor.h"
#include "lz78rule.h"
#include "lzwrule.h"
#include "lz_compressor.h"
#include "lz78.h"
#include "lzw.h"
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

// Algorithm interfaces

using esacomp::EsacompRuleCompressor;
using esacomp::EsacompCompressStrategy;
using esacomp::EsacompEncodeStrategy;

using lz78::Lz78Rule;
using lz78::Lz78RuleCoder;

using lzw::LzwRule;
using lzw::LzwRuleCoder;

// Algorithm implementations

using lz78::LZ78DebugCode;
using lz78::LZ78BitCode;

using lzw::LZWDebugCode;
using lzw::LZWBitCode;

using esacomp::LZCompressor;
using esacomp::DummyCompressor;
using esacomp::DummyCoder;

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
    registry.with_info<EsacompRuleCompressor>(
        // TODO: Name right
        // - lz77rule ~ LZSS rules with implicit escape bits
        // - lz ~ unknown named variant of lz with suffix array
        "Esacomp rules", "esacomp",
        "A Family of compression algorithms making use "
        "of esacomp-like replacement rules.")
    .with_sub_algos<EsacompCompressStrategy>([](AlgorithmRegistry<EsacompCompressStrategy>& registry) {
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

    })
    .with_sub_algos<EsacompEncodeStrategy>([](AlgorithmRegistry<EsacompEncodeStrategy>& registry) {
        registry.set_name("Coder");

        registry.with_info<DummyCoder>(
            "Dummy", "dummy",
            "Dummy encoding, outputs input text unchanged.").do_register();

        registry.with_info<Code0Coder>(
            "Code0", "debug",
            "Human readable output of text and rules.").do_register();

        registry.with_info<Code1Coder>(
            "Code1", "code1",
            "Byte based encoding.").do_register();

        registry.with_info<Code2Coder>(
            "Code2", "code2",
            "Bit based encoding.").do_register();
    })
    .do_register();

    registry.with_info<Lz78Rule>(
        "LZ78 rule-like", "lz78rule",
        "A Family of compression algorithms making use "
        "of LZ78-like replacement rules.")
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

    registry.with_info<LzwRule>(
        "LZW rule-like", "lzwrule",
        "A Family of compression algorithms making use "
        "of LZW-like dictionary entries.")
    .with_sub_algos<LzwRuleCoder>([](AlgorithmRegistry<LzwRuleCoder>& registry) {
        registry.set_name("Coder");

        registry.with_info<LZWDebugCode>(
            "Debug", "debug",
            "Debug encoding, each rule is emitted as a string of the form `<idx>,...` where <idx> is either a integer or a char literal").do_register();

        registry.with_info<LZWBitCode>(
            "Bit", "bit",
            "Bit encoding, each rule is emitted a bitstream of the "
            "minimum amount of bits needed to encode the index and the char").do_register();
    })
    .do_register();
}

}
