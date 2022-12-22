#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/grammar/Grammar.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/grammar/GrammarCoding.hpp>
namespace tdc {
namespace grammar {

class DidacticGrammarCoder : public Algorithm {

public:
    inline static Meta meta() {
        Meta m(
            grammar_coder_type(),
            "didactic_grammar",
            "Didactic output of grammars"
        );
        return m;
    }/* message */

    using Algorithm::Algorithm;

    class Encoder {
    private:
        std::unique_ptr<io::OutputStream> m_out;

    public:
        inline Encoder(const Config& cfg, Output& out) {
            m_out = std::make_unique<io::OutputStream>(out.as_stream());
        }

        inline void encode_grammar(Grammar &grammar) {
            grammar.print(*m_out);
        } 
    };

    class Decoder {
    private:
        std::unique_ptr<io::InputStream> m_in;

    public:
        inline Decoder(const Config& cfg, Input& in) {
            m_in = std::make_unique<io::InputStream>(in.as_stream());
        }

        inline Grammar decode_grammar() {
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