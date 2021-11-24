#pragma once

#include <tudocomp/Decompressor.hpp>
#include <tudocomp/meta/Meta.hpp>
#include <tudocomp/grammar/GrammarCoding.hpp>

namespace tdc {
namespace grammar {

template<typename grammar_coder_t> 
class DefaultGrammarDecompressor : public Decompressor {
private:
    
public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "default_grammar_decomp",
            "Decompresses a grammar");
        m.param("coder", "The input decoder.")
            .strategy<grammar_coder_t>(grammar_coder_type());
        return m;
    }
    
    using Decompressor::Decompressor;
    
    void decompress(Input& input, Output& output) override {
        typename grammar_coder_t::Decoder coder(config().sub_config("coder"), input);
        Grammar gr = coder.decode_grammar();
        output.as_stream() << gr.reproduce();
    }
};

}}