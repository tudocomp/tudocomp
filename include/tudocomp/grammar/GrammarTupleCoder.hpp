#pragma once

#include <tudocomp/Coder.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/grammar/GrammarCoding.hpp>

namespace tdc {
namespace grammar {
    


template<typename len_coder_t, typename terminal_coder_t, typename nonterminal_coder_t>
class GrammarTupleCoder : public Algorithm {
    
public:
    inline static Meta meta() {
        Meta m(grammar_coder_type(), "grammar_tuple",
            "Encodes grammar rules as tuples of rule length and symbols");
        m.param("len_coder", "The encoder encoding the length values.")
            .strategy<len_coder_t>(Coder::type_desc());
        m.param("terminal_coder", "The encoder encoding terminals.")
            .strategy<terminal_coder_t>(Coder::type_desc());
        m.param("nonterminal_coder", "The encoder encoding non terminals.")
            .strategy<nonterminal_coder_t>(Coder::type_desc());
        return m;
    }
    
    
    //template<typename len_coder_t, typename terminal_coder_t, typename nonterminal_coder_t>
    class Encoder {         
        std::shared_ptr<BitOStream> m_out;
        std::unique_ptr<typename len_coder_t::Encoder> m_len_encoder;        
        std::unique_ptr<typename terminal_coder_t::Encoder> m_terminal_encoder;        
        std::unique_ptr<typename nonterminal_coder_t::Encoder> m_nonterminal_encoder;        
    public:
        
        inline Encoder(const Config& cfg, Output& out) {
            m_out = std::make_shared<BitOStream>(out);
            m_len_encoder = std::make_unique<typename len_coder_t::Encoder>(cfg.sub_config("len_coder"), m_out, NoLiterals());
            m_terminal_encoder = std::make_unique<typename terminal_coder_t::Encoder>(cfg.sub_config("terminal_coder"), m_out, NoLiterals());
            m_nonterminal_encoder = std::make_unique<typename nonterminal_coder_t::Encoder>(cfg.sub_config("nonterminal_coder"), m_out, NoLiterals());
        }

        void encode_grammar(Grammar &grammar) {
            if (grammar.empty()) {
                return;
            }

            grammar.dependency_renumber();

            size_t min_len = std::numeric_limits<size_t>().max();
            size_t max_len = std::numeric_limits<size_t>().min();
            for (auto rule : *grammar) {
                min_len = std::min(min_len, rule.second.size());
                max_len = std::max(max_len, rule.second.size());
            }
            

            Range rule_len_r = Range(min_len, max_len);
            m_len_encoder->template encode<size_t>(min_len, size_r);
            m_len_encoder->template encode<size_t>(max_len, size_r);

            size_t current_rule_id = 0;
            for (auto it = (*grammar).begin(); it != (*grammar).end(); it++) {
                
                auto symbols = (*it).second;
                auto length = symbols.size();
                m_len_encoder->encode(length, rule_len_r);
                
                for(size_t i = 0; i < symbols.size(); i++){
                    auto symbol = symbols[i];
                    if(Grammar::is_terminal(symbol)) {
                        m_out->write_bit(false);
                        m_terminal_encoder->encode((char) symbol, uliteral_r);
                        continue;
                    }
                    m_out->write_bit(true);
                    m_nonterminal_encoder->encode(symbol - Grammar::RULE_OFFSET, Range(0, current_rule_id));
                }
                current_rule_id++;
            }
        }
    };

    
    class Decoder {
        
        std::shared_ptr<BitIStream> m_in;
        std::unique_ptr<typename len_coder_t::Decoder> m_len_decoder;        
        std::unique_ptr<typename terminal_coder_t::Decoder> m_terminal_decoder;        
        std::unique_ptr<typename nonterminal_coder_t::Decoder> m_nonterminal_decoder;        
         
    public:
        inline Decoder(const Config& cfg, Input& in) {
            m_in = std::make_shared<BitIStream>(in.as_stream());
            m_len_decoder = std::make_unique<typename len_coder_t::Decoder>(cfg.sub_config("len_coder"), m_in);
            m_terminal_decoder = std::make_unique<typename terminal_coder_t::Decoder>(cfg.sub_config("terminal_coder"), m_in);
            m_nonterminal_decoder = std::make_unique<typename nonterminal_coder_t::Decoder>(cfg.sub_config("nonterminal_coder"), m_in);
        }
    
        Grammar decode_grammar() {
            Grammar gr;
            if (m_in->eof()) return gr;

            size_t min_len = m_len_decoder->template decode<size_t>(size_r);
            size_t max_len = m_len_decoder->template decode<size_t>(size_r);
            Range rule_len_r(min_len, max_len);

            size_t current_rule_id = 0;
            while(!m_in->eof()) {

                size_t rule_len = m_len_decoder->template decode<size_t>(rule_len_r);
                for (size_t i = 0; i < rule_len; i++) {
                    bool is_nonterminal = m_in->read_bit();
                    if(!is_nonterminal) {
                        char terminal = m_terminal_decoder->template decode<char>(uliteral_r);
                        gr.append_terminal(current_rule_id, terminal);
                        continue;
                    }

                    size_t rule_id = m_nonterminal_decoder->template decode<size_t>(Range(0, current_rule_id));
                    gr.append_nonterminal(current_rule_id, rule_id);
                }
                current_rule_id++;
            }
            gr.set_start_rule_id(current_rule_id - 1);
            return gr;
        }
    };
        
    inline auto encoder(Output& output) {
        return Encoder(config(), output);
    }

    inline auto decoder(Input& input) {
        return Decoder(config(), input);
    }
};

} // namespace grammar
} // namespace tdc