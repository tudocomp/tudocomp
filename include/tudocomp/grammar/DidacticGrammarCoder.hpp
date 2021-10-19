#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/grammar/Grammar.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/grammar/GrammarCoding.hpp>
namespace tdc {
namespace grammar {

class DidacticGrammarEncoder : public Algorithm {

public:
    inline static Meta meta() {
        Meta m(
            grammar_coder_type(),
            "didactic_grammar",
            "Didactic output of grammars"
        );
        return m;
    }

    using Algorithm::Algorithm;

    class Encoder {
    private:
        std::unique_ptr<io::OutputStream> m_out;

    public:
        inline Encoder(const Config& cfg, Output& out) {
            m_out = std::make_unique<io::OutputStream>(out.as_stream());
        }

        inline void encode_grammar(Grammar &grammar) {
            for (auto &&pair : *grammar)
            {
                const auto id = pair.first;
                const auto symbols = pair.second;
                (*m_out) << 'R' << id << " -> ";
                for (auto &&symbol : symbols) {
                    if (symbol < Grammar::RULE_OFFSET) { // in this case it's a terminal
                        (*m_out) << (char) symbol;
                    } else {
                        (*m_out) << 'R' << symbol - Grammar::RULE_OFFSET;
                    }
                    (*m_out) << ' ';
                }
                (*m_out) << std::endl;
            }
            
        } 
    };

    class Decoder {
    private:
        std::unique_ptr<io::InputStream> m_in;

    public:
        inline Decoder(const Config& cfg, Input& in) {
            m_in = std::make_unique<io::InputStream>(in.as_stream());
        }

        template<typename decomp_t>
        inline void decode(decomp_t& decomp) {
            throw std::runtime_error("not implemented");
        }
    };

    inline auto encoder(Output &output) {
        return Encoder(config(), output);
    }

    inline auto decoder(Input& input) {
        return Decoder(config(), input);
    }
};

}}