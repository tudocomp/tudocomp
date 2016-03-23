#include <tudocomp_driver/registry.h>

#include <tudocomp/lz78/lz78_compressor.h>
#include <tudocomp/lz78/dummy_coder.h>
#include <tudocomp/lz78/bit_coder.h>

#include <tudocomp/lzw/lzw_compressor.h>
#include <tudocomp/lzw/dummy_coder.h>
#include <tudocomp/lzw/bit_coder.h>

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

// Algorithm interfaces

//TODO: esacomp needs to be re-inserted!
/*
using esacomp::EsacompRuleCompressor;
using esacomp::EsacompCompressStrategy;
using esacomp::EsacompEncodeStrategy;
*/

using ::lz78::Lz78Rule;
using ::lz78::Lz78RuleCoder;

using ::lzw::LzwRule;
using ::lzw::LzwRuleCoder;

// Algorithm implementations

using ::lz78::LZ78DebugCode;
using ::lz78::LZ78BitCode;

using ::lzw::LZWDebugCode;
using ::lzw::LZWBitCode;

using ::tudocomp::lzw::LzwCompressor;
using ::tudocomp::lzw::LzwDebugCoder;
using ::tudocomp::lzw::LzwBitCoder;

using ::tudocomp::lz78::Lz78Compressor;
using ::tudocomp::lz78::Lz78DebugCoder;
using ::tudocomp::lz78::Lz78BitCoder;

using ::tudocomp::lz78::lzcics::Lz78cicsCompressor;

//TODO: esacomp needs to be re-inserted!
/*
using esacomp::LZCompressor;
using esacomp::DummyCompressor;
using esacomp::DummyCoder;
*/

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

void foo();
void register_algos(AlgorithmRegistry<Compressor>& registry) {
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

    registry.with_info<Lz78Compressor<Lz78DebugCoder>>(
        "lz78 debug coder", "t_lz78_debug",
        "Template-based implementation of a lz78 coder with human readable"
        "debug output")
    .do_register();
    registry.with_info<Lz78Compressor<Lz78BitCoder>>(
        "lz78 bit coder", "t_lz78_bit",
        "Template-based implementation of a lz78 coder with naive bit"
        "compressed output")
    .do_register();
    registry.with_info<LzwCompressor<LzwDebugCoder>>(
        "lzw debug coder", "t_lzw_debug",
        "Template-based implementation of a lzw coder with human readable"
        "debug output")
    .do_register();
    registry.with_info<LzwCompressor<LzwBitCoder>>(
        "lzw bit coder", "t_lzw_bit",
        "Template-based implementation of a lzw coder with naive bit"
        "compressed output")
    .do_register();
    registry.with_info<Lz78cicsCompressor<Lz78BitCoder>>(
        "lz78 cics bit", "t_lz78_cics_bit",
        "")
    .do_register();
    registry.with_info<Lz78cicsCompressor<Lz78DebugCoder>>(
        "lz78 cics debug", "t_lz78_cics_debug",
        "")
    .do_register();

    foo();
}

template<class C>
C make(Env& env) {
    return C(env);
}

struct baz;

struct bar {
    AlgorithmDb* m_algorithms;

    std::unordered_map<
        std::string,
        std::function<std::unique_ptr<Compressor>(Env&)>> compressors;

    // AlgorithmInfo

    template<class F>
    void compressor(std::string id, F f) {
        auto g = [=](Env& env) {
            using C = decltype(f(env));

            return std::unique_ptr<Compressor> {
                new C(f(env))
            };
        };

        compressors[id] = g;
    }

    baz algo(std::string id, std::string title, std::string desc);

    std::vector<std::string> check_for_undefined_compressors() {
        std::vector<std::string> r;
        for (auto& s : m_algorithms->id_product()) {
            if (compressors.count(s) == 0) {
                r.push_back(s);
            }
        }
        return r;
    }
};

struct baz {
    std::vector<AlgorithmInfo>* m_vector;
    int m_index;

    template<class G>
    void sub_algo(std::string name, G g) {
        auto& x = (*m_vector)[m_index].sub_algo_info;
        x.push_back({name});
        bar y { &x[x.size() - 1] };
        g(y);
    }

};

baz bar::algo(std::string id, std::string title, std::string desc) {
    m_algorithms->sub_algo_info.push_back({title, id, desc});

    return baz {
        &m_algorithms->sub_algo_info,
        m_algorithms->sub_algo_info.size() - 1
    };
}

void foo() {
    AlgorithmDb m_algorithms { "root" };
    bar b {&m_algorithms};

    //b.compressor("a.b.c", make<A<B, C>>);

    b.algo("lzw", "Lempel-Ziv-Welch algorithm", "")
        .sub_algo("Coder", [](bar& b) {
            b.algo("bit", "Bit coder", "basic variable-bit-width encoding of the symbols");
            b.algo("debug", "Debug coder", "human readable, comma separated stream of integers");
        });
    b.algo("lz78", "Lempel-Ziv 78 algorithm", "")
        .sub_algo("Coder", [](bar& b) {
            b.algo("bit", "Bit coder", "basic variable-bit-width encoding of the symbols");
            b.algo("debug", "Debug coder", "human readable, comma separated stream of (integer, char) tuples");
        });

    b.compressor("lzw.debug", make<LzwCompressor<LzwDebugCoder>>);
    b.compressor("lzw.bit", make<LzwCompressor<LzwBitCoder>>);

    b.m_algorithms->print_to(std::cout, 0);
    std::cout << "Undefined:\n";
    for (auto& s: b.check_for_undefined_compressors()) {
        std::cout << s << "\n";
    }

    //b.compressor("esa.esa.code0")
    //b.compressor("esa.esa.code1")
    //b.compressor("esa.lz.code0")
    //b.compressor("esa.lz.code1")

}

}
