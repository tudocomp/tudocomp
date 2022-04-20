#pragma once

#include "tudocomp/compressors/areacomp/areafunctions/AreaFunction.hpp"
#include <iostream>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/compressors/areacomp/ChildArray.hpp>
#include <tudocomp/compressors/areacomp/areafunctions/HeightFirstArea.hpp>
#include <tudocomp/decompressors/DefaultGrammarDecompressor.hpp>
#include <tudocomp/decompressors/WrapDecompressor.hpp>
#include <tudocomp/grammar/DidacticGrammarCoder.hpp>
#include <tudocomp/grammar/Grammar.hpp>
#include <tudocomp/grammar/GrammarCoding.hpp>
#include <tudocomp/meta/Meta.hpp>

#include <tudocomp/ds/DSManager.hpp>
#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/ds/providers/ISAFromSA.hpp>
#include <tudocomp/ds/providers/LCPFromPLCP.hpp>
#include <tudocomp/ds/providers/PhiAlgorithm.hpp>
#include <tudocomp/ds/providers/PhiFromSA.hpp>

#include <tudocomp/compressors/areacomp/Ruleset.hpp>

namespace tdc {

template<typename grammar_coder_t,
         template<typename> class area_fun_t = grammar::areacomp::HeightFirstArea,
         // size_t sampling = 16,
         typename ds_t = DSManager<DivSufSort, PhiFromSA, PhiAlgorithm, LCPFromPLCP, ISAFromSA>>
requires grammar::areacomp::AreaFun<area_fun_t<ds_t>, ds_t>
class AreaCompressor : public Compressor {

  public:
    inline static Meta meta() {
        // TODO Figure out how to correctly configure the Meta object
        Meta m(Compressor::type_desc(), "areacomp", "Grammar Compression using AreaComp");
        m.param("coder", "The output grammar encoder.").strategy<grammar_coder_t>(grammar::grammar_coder_type());
        m.param("area_function", "The area function to use.")
            .strategy<area_fun_t<ds_t>>(grammar::areacomp::area_fun_type_desc());
        // m.param("sampling", "The sampling value of the predecessor structure. Buckets are the size of 2 raised to the
        // power of this value")
        //   .primitive<sampling>(16);
        m.param("min_area", "Only lcp intervals with area values greater than this value will be considered")
            .primitive(0);
        m.param("ds", "The text data structure provider.")
            .strategy<ds_t>(ds::type(),
                            Meta::Default<DSManager<DivSufSort, PhiFromSA, PhiAlgorithm, LCPFromPLCP, ISAFromSA>>());

        return m;
    }

    using Compressor::Compressor;

    inline grammar::Grammar build_grammar(Input &input, StatPhase &phase = StatPhase("dummy")) {
        using tdc::grammar::Grammar;

        auto in = input.as_view();

        auto min_area = config().param("min_area").as_uint();

        // Get the area function
        area_fun_t<ds_t> area_fun(config().sub_config("area_function"));

        // Construct text data structures
        ds_t text(config().sub_config("ds"), in);
        text.template construct<ds::SUFFIX_ARRAY, ds::LCP_ARRAY, ds::INVERSE_SUFFIX_ARRAY>();

        // Create a rule set and compress it using the area function
        grammar::areacomp::Ruleset<16> rules(in, min_area);

        phase.split("Compress Ruleset");

        rules.compress(area_fun, text);

        phase.split("Convert to Grammar");

        return rules.build_grammar();
    }

    inline void compress(Input &input, Output &output) override {
        using tdc::grammar::Grammar;

        StatPhase phase("Text data structures");
        // Convert the ruleset to a grammar
        Grammar gr = this->build_grammar(input, phase);

        phase.split("Encode Grammar");

        phase.log_stat("grammar_size", gr.grammar_size());
        phase.log_stat("rule_count", gr.rule_count());

        // Encode the grammar
        typename grammar_coder_t::Encoder coder(config().sub_config("coder"), output);
        coder.encode_grammar(gr);
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::instance<DefaultGrammarDecompressor<grammar_coder_t>>();
    }
};

} // namespace tdc
