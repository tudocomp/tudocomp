#pragma once

#include <tudocomp/grammar/sequitur/classes.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/meta/Meta.hpp>
#include <tudocomp/grammar/GrammarCoding.hpp>
#include <tudocomp/grammar/Grammar.hpp>
#include <tudocomp/grammar/sequitur/sequitur_vars.hpp>
#include <tudocomp/grammar/DidacticGrammarCoder.hpp>

#include <deque>
#include <unordered_set>

namespace tdc {
namespace grammar {

template<typename grammar_coder_t=DidacticGrammarEncoder>
class SequiturCompressor : public CompressorAndDecompressor {

public:
    inline static Meta meta() {
        Meta m(
            Compressor::type_desc(),
            "sequitur",
            "Grammar Compression using Sequitur"
        );
        m.param("min_occurrences", "The minimum number of times a digram must occur to form a rule")
            .primitive(2);
        m.param("delimiter", "Rules will not be formed across this ")
            .primitive(-1);
        m.param("coder", "The output grammar encoder.")
            .strategy<grammar_coder_t>(grammar_coder_type());
        return m;
    }

    using CompressorAndDecompressor::CompressorAndDecompressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_stream();
        sequitur::reset_vars();
        
        const auto min_occurrences = config().param("min_occurrences").as_uint();
        if (min_occurrences < 1) {
            std::cerr << "min_occurrences must at least be 2" << std::endl;
            return;
        } 

        const char delimiter = config().param("delimiter").as_uint();

        sequitur::delimiter = delimiter;
        sequitur::K = min_occurrences - 1;

        auto s = new sequitur::rules;
        s->last()->insert_after(new sequitur::symbols(in.get()));


        while (!in.eof()) {
            const auto c = in.get();
            if (c == -1) break;
            s->last()->insert_after(new sequitur::symbols(c));
            s->last()->prev()->check();           
        }

        s->number_rules_recursive();

        tdc::grammar::Grammar grammar;
        grammar.set_start_rule_id(0);

        std::deque<sequitur::rules*> to_be_processed;
        std::unordered_set<size_t> seen;

        to_be_processed.push_back(s);
        
        while (!to_be_processed.empty()) {
            sequitur::rules* current_rule = to_be_processed.at(0);
            to_be_processed.pop_front();
            const auto current_rule_id = current_rule->index();
            for (auto current_symbol = current_rule->first(); !current_symbol->is_guard(); current_symbol = current_symbol->next()) {
                if (current_symbol->non_terminal()) {
                    const auto rule = current_symbol->rule();
                    if (seen.find(rule->index()) == seen.end()) {
                        to_be_processed.push_back(rule);
                        seen.insert(rule->index());
                    }
                    grammar.append_nonterminal(current_rule_id, rule->index());
                } else {
                    grammar.append_terminal(current_rule_id, current_symbol->value());
                }
            } 
            std::cout << std::endl;
        }
        
        typename grammar_coder_t::Encoder coder(config().sub_config("coder"), output);
        coder.encode_grammar(grammar);
        delete s; 
    }
        
    virtual void decompress(Input& input, Output& output) override {
        
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return std::make_unique<WrapDecompressor>(*this);
    }
};

}}