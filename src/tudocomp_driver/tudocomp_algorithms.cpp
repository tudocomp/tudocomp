#include <tudocomp_driver/registry.h>

#include <tudocomp/lz78/Lz78Compressor.hpp>
#include <tudocomp/lz78/Lz78DebugCoder.hpp>
#include <tudocomp/lz78/Lz78BitCoder.hpp>

#include <tudocomp/lzw/LzwCompressor.hpp>
#include <tudocomp/lzw/LzwDebugCoder.hpp>
#include <tudocomp/lzw/LzwBitCoder.hpp>

#include <tudocomp/lz78/lzcics/Lz78cicsCompressor.hpp>

//TODO: esacomp needs to be re-inserted!
/*
#include "lz_compressor.h"
#include "esacomp/esacomp_rule_compressor.h"
#include "esa_compressor.h"
#include "max_lcp_sorted_suffix_list.h"
#include "max_lcp_heap.h"
#include "dummy_compressor.h"

#include "code0.h"
#include "code1.h"
#include "code2.h"
#include "dummy_encoder.h"
*/

namespace tudocomp_driver {

using namespace tudocomp;

// Algorithm implementations

//TODO: esacomp needs to be re-inserted!
/*
using esacomp::EsacompRuleCompressor;
using esacomp::EsacompCompressStrategy;
using esacomp::EsacompEncodeStrategy;
*/

using lzw::LzwCompressor;
using lzw::LzwDebugCoder;
using lzw::LzwBitCoder;

using lz78::Lz78Compressor;
using lz78::Lz78DebugCoder;
using lz78::Lz78BitCoder;

using lz78::lzcics::Lz78cicsCompressor;

//TODO: esacomp needs to be re-inserted!
/*
using esacomp::LZCompressor;
using esacomp::DummyCompressor;
using esacomp::DummyCoder;
*/

/// Small helper for getting a constructor function
template<class C>
C make(Env& env) {
    return C(env);
}

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

void register_algos(Registry& r) {
    //TODO: esacomp needs to be re-inserted!
    /*
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
    .do_register();*/

    r.algo("lz78", "Lempel-Ziv 78 algorithm", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("bit", "Bit coder", "basic variable-bit-width encoding of the symbols");
            r.algo("debug", "Debug coder", "human readable, comma separated stream of (integer, char) tuples");
        });
    r.algo("lzw", "Lempel-Ziv-Welch algorithm", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("bit", "Bit coder", "basic variable-bit-width encoding of the symbols");
            r.algo("debug", "Debug coder", "human readable, comma separated stream of integers");
        });
    r.algo("lz78_cics", "Lempel-Ziv 78 algorithm", "")
        .sub_algo("Coder", [](Registry& r) {
            r.algo("bit", "Bit coder", "basic variable-bit-width encoding of the symbols");
            r.algo("debug", "Debug coder", "human readable, comma separated stream of integers");
        });

    r.compressor("lzw.debug", make<LzwCompressor<LzwDebugCoder>>);
    r.compressor("lzw.bit", make<LzwCompressor<LzwBitCoder>>);
    r.compressor("lz78.debug", make<Lz78Compressor<Lz78DebugCoder>>);
    r.compressor("lz78.bit", make<Lz78Compressor<Lz78BitCoder>>);
    r.compressor("lz78_cics.debug", make<Lz78cicsCompressor<Lz78DebugCoder>>);
    r.compressor("lz78_cics.bit", make<Lz78cicsCompressor<Lz78BitCoder>>);

}

}
