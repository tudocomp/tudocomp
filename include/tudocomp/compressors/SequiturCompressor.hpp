#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/decompressors/DefaultGrammarDecompressor.hpp>
#include <tudocomp/decompressors/WrapDecompressor.hpp>
#include <tudocomp/grammar/DidacticGrammarCoder.hpp>
#include <tudocomp/grammar/Grammar.hpp>
#include <tudocomp/grammar/GrammarCoding.hpp>
#include <tudocomp/grammar/sequitur/classes.hpp>
#include <tudocomp/grammar/sequitur/sequitur_vars.hpp>
#include <tudocomp/meta/Meta.hpp>
#include <tudocomp_stat/StatPhase.hpp>

#include <deque>
#include <unordered_set>

namespace tdc {

using namespace grammar;

/**
 * @brief Compresses the input using the Sequitur grammar compressor.
 * This algorithm is described by Nevill-Manning et al. in "Identifying hierarchical structure in sequences: A
 * linear-time algorithm" (https://arxiv.org/abs/cs/9709102)
 *
 * @tparam grammar_coder_t The coder to use for encoding and decoding the resulting Grammar
 */
template<typename grammar_coder_t>
class SequiturCompressor : public Compressor {

  public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "sequitur", "Grammar Compression using Sequitur");
        m.param("min_occurrences", "The minimum number of times a digram must occur to form a rule").primitive(2);
        m.param("delimiter", "Rules will not be formed across this ").primitive(-1);
        m.param("coder", "The output grammar encoder.").strategy<grammar_coder_t>(grammar_coder_type());
        return m;
    }

    using Compressor::Compressor;

    inline grammar::Grammar build_grammar(Input &input, StatPhase &phase = StatPhase("dummy")) {

        phase.split("Initialization");

        sequitur::reset_vars();

        const auto min_occurrences = config().param("min_occurrences").as_uint();
        if (min_occurrences < 1) {
            std::cerr << "min_occurrences must at least be 2" << std::endl;
            return Grammar(0);
        }

        // Rules will not be formed across this character
        const char delimiter = config().param("delimiter").as_uint();

        sequitur::delimiter = delimiter;
        sequitur::K         = min_occurrences - 1;

        auto in = input.as_stream();

        phase.split("Sequitur");

        // The starting rule
        auto s = new sequitur::rules;
        s->last()->insert_after(new sequitur::symbols(in.get()));

        while (!in.eof()) {
            const auto c = in.get();
            if (c == -1)
                break;
            s->last()->insert_after(new sequitur::symbols(c));
            s->last()->prev()->check();
        }

        phase.split("Renumber Rules");

        // Renumber the rules to occupy ids 0 to k, where k is the number of rules in the grammar
        size_t max_rule = s->number_rules_recursive();

        phase.split("Transform Grammar into tdc representation");

        // Transform the Sequitur grammar into the tdc representation
        tdc::grammar::Grammar grammar(max_rule + 1);
        grammar.set_start_rule_id(s->index());

        {

            // The rules marked for processing
            std::deque<sequitur::rules *> to_be_processed;
            // The rules which have already been processed
            std::unordered_set<size_t> seen;

            // The starting rule is the first to be processed
            to_be_processed.push_back(s);

            while (!to_be_processed.empty()) {

                // Take the next rule to be processed out of the deque
                sequitur::rules *current_rule = to_be_processed.at(0);
                to_be_processed.pop_front();
                const auto current_rule_id = current_rule->index();

                // Iterate through the symbols of the rule
                for (auto current_symbol = current_rule->first(); !current_symbol->is_guard();
                     current_symbol      = current_symbol->next()) {
                    // If the current symbol is a non-terminal, get the corresponding rule id and check if it has been
                    // processed yet If it has not been processed yet, add it to the deque and the set In both cases,
                    // add the current non-terminal to the grammar
                    if (current_symbol->non_terminal()) {
                        const auto rule = current_symbol->rule();
                        if (seen.find(rule->index()) == seen.end()) {
                            to_be_processed.push_back(rule);
                            seen.insert(rule->index());
                        }
                        grammar.append_nonterminal(current_rule_id, rule->index());
                    } else {
                        // If the current symbol is a terminal we can just add it to the rule in the grammar
                        grammar.append_terminal(current_rule_id, current_symbol->value());
                    }
                }
            }
        }
        delete s;

        return grammar;
    }

    inline virtual void compress(Input &input, Output &output) override {

        StatPhase seq_phase("Sequitur");

        auto grammar = build_grammar(input, seq_phase);

        seq_phase.split("Encode Grammar");

        typename grammar_coder_t::Encoder coder(config().sub_config("coder"), output);
        coder.encode_grammar(grammar);
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::instance<DefaultGrammarDecompressor<grammar_coder_t>>();
    }
};

} // namespace tdc
