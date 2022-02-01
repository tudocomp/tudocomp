#pragma once

#include <tudocomp/Coder.hpp>
#include <tudocomp/grammar/Grammar.hpp>
#include <tudocomp/grammar/GrammarCoding.hpp>
#include <tudocomp/io.hpp>

namespace tdc {

/**
 * @brief Codes a grammar's rules into tuples of their length in symbols and a list of their symbols.
 *
 * In the encoded symbol list a 0 bit precedes a terminal and a 1 bit precedes a non-terminal.
 * The rules are encoded in an order which rules that depend on no other rule (i.e. contains only terminals) are encoded
 * first. For a rule to be encoded at a certain point in time, all rules which this rule depends on, have to be encoded
 * already. This allows for all encoded non-terminals to be references to rules which have already be en- or decoded.
 *
 * @tparam len_coder_t The coder to use for the length of the rules
 * @tparam terminal_coder_t The coder to use for terminals
 * @tparam nonterminal_coder_t The coder to use for non-terminals
 */
template<typename len_coder_t, typename terminal_coder_t, typename nonterminal_coder_t>
class GrammarTupleCoder : public Algorithm {

  public:
    inline static Meta meta() {
        Meta m(grammar::grammar_coder_type(),
               "grammar_tuple",
               "Encodes grammar rules as tuples of rule length and symbols");
        m.param("len_coder", "The encoder encoding the length values.").strategy<len_coder_t>(Coder::type_desc());
        m.param("terminal_coder", "The encoder encoding terminals.").strategy<terminal_coder_t>(Coder::type_desc());
        m.param("nonterminal_coder", "The encoder encoding non terminals.")
            .strategy<nonterminal_coder_t>(Coder::type_desc());
        return m;
    }

    class Encoder {
        std::shared_ptr<BitOStream>                            m_out;
        std::unique_ptr<typename len_coder_t::Encoder>         m_len_encoder;
        std::unique_ptr<typename terminal_coder_t::Encoder>    m_terminal_encoder;
        std::unique_ptr<typename nonterminal_coder_t::Encoder> m_nonterminal_encoder;

      public:
        inline Encoder(const Config &cfg, Output &out) {
            m_out = std::make_shared<BitOStream>(out);
            m_len_encoder =
                std::make_unique<typename len_coder_t::Encoder>(cfg.sub_config("len_coder"), m_out, NoLiterals());
            m_terminal_encoder = std::make_unique<typename terminal_coder_t::Encoder>(cfg.sub_config("terminal_coder"),
                                                                                      m_out,
                                                                                      NoLiterals());
            m_nonterminal_encoder =
                std::make_unique<typename nonterminal_coder_t::Encoder>(cfg.sub_config("nonterminal_coder"),
                                                                        m_out,
                                                                        NoLiterals());
        }

        void encode_grammar(grammar::Grammar &grammar) {
            if (grammar.empty()) {
                return;
            }

            // Renumber the rules, such that the rules with and id n only depends on rules with id k < n
            grammar.dependency_renumber();

            // Determine the maximum and minimum rule length in the grammar respectively
            size_t min_len = std::numeric_limits<size_t>::max();
            size_t max_len = std::numeric_limits<size_t>::min();
            for (size_t rule_id = 0; rule_id < grammar.grammar_size(); rule_id++) {
                auto &symbols = grammar[rule_id];
                min_len       = std::min(min_len, symbols.size());
                max_len       = std::max(max_len, symbols.size());
            }

            // Encode the rule count
            m_len_encoder->template encode<size_t>(grammar.rule_count(), size_r);

            // Encode the minimum and maximum rule lengths
            Range rule_len_r = Range(min_len, max_len);
            m_len_encoder->template encode<size_t>(min_len, size_r);
            m_len_encoder->template encode<size_t>(max_len, size_r);

            size_t n = grammar.rule_count();
            // Iterate through the grammar
            for (size_t current_rule_id = 0; current_rule_id < n; ++current_rule_id) {
                auto symbols = grammar[current_rule_id];
                // Encode the rule's length
                auto length = symbols.size();
                m_len_encoder->encode(length, rule_len_r);
                // Iterate through the rule's symbols
                for (auto symbol : symbols) {
                    // If the symbol is a terminal, write a 0 bit and then encode the symbol with the terminal encoder
                    if (grammar::Grammar::is_terminal(symbol)) {
                        m_out->write_bit(false);
                        m_terminal_encoder->encode((char) symbol, uliteral_r);
                        continue;
                    }
                    // If the symbol is a nonterminal, write a 1 bit and then encode the rule id with the nonterminal
                    // encoder
                    m_out->write_bit(true);
                    m_nonterminal_encoder->encode(symbol - grammar::Grammar::RULE_OFFSET, Range(0, current_rule_id));
                }
            }
        }
    };

    class Decoder {

        std::shared_ptr<BitIStream>                            m_in;
        std::unique_ptr<typename len_coder_t::Decoder>         m_len_decoder;
        std::unique_ptr<typename terminal_coder_t::Decoder>    m_terminal_decoder;
        std::unique_ptr<typename nonterminal_coder_t::Decoder> m_nonterminal_decoder;

      public:
        inline Decoder(const Config &cfg, Input &in) {
            m_in          = std::make_shared<BitIStream>(in.as_stream());
            m_len_decoder = std::make_unique<typename len_coder_t::Decoder>(cfg.sub_config("len_coder"), m_in);
            m_terminal_decoder =
                std::make_unique<typename terminal_coder_t::Decoder>(cfg.sub_config("terminal_coder"), m_in);
            m_nonterminal_decoder =
                std::make_unique<typename nonterminal_coder_t::Decoder>(cfg.sub_config("nonterminal_coder"), m_in);
        }

        grammar::Grammar decode_grammar() {
            // If there is nothing to read, just return the empty grammar
            if (m_in->eof())
                return grammar::Grammar(0);

            size_t rule_count = m_len_decoder->template decode<size_t>(size_r);

            grammar::Grammar gr(rule_count);
            // Decode the minimum and maximum rule lengths
            size_t min_len = m_len_decoder->template decode<size_t>(size_r);
            size_t max_len = m_len_decoder->template decode<size_t>(size_r);
            Range  rule_len_r(min_len, max_len);

            size_t current_rule_id = 0;
            while (!m_in->eof()) {

                // Decode the rule length
                size_t rule_len = m_len_decoder->template decode<size_t>(rule_len_r);
                // Decode rule_len symbols
                for (size_t i = 0; i < rule_len; i++) {
                    // Read a bit from the input. If it is 1, the next symbol is a nonterminal. If it is 0, it is a
                    // terminal
                    bool is_nonterminal = m_in->read_bit();
                    // If it is a terminal, use the terminal decoder to decode the next value as a character and insert
                    // it into the grammar
                    if (!is_nonterminal) {
                        char terminal = m_terminal_decoder->template decode<char>(uliteral_r);
                        gr.append_terminal(current_rule_id, terminal);
                        continue;
                    }
                    // If it is a nonterminal read the rule id from the grammar and insert it into the grammar
                    size_t rule_id = m_nonterminal_decoder->template decode<size_t>(Range(0, current_rule_id));
                    gr.append_nonterminal(current_rule_id, rule_id);
                }
                current_rule_id++;
            }
            // The rule that was read last is the one with the most dependencies. That rule is the start rule.
            gr.set_start_rule_id(current_rule_id - 1);
            return gr;
        }
    };

    inline auto encoder(Output &output) { return Encoder(config(), output); }

    inline auto decoder(Input &input) { return Decoder(config(), input); }
};

} // namespace tdc
