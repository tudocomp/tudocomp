#ifndef _INCLUDED_LITERAL_ENCODER_HPP_
#define _INCLUDED_LITERAL_ENCODER_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/io.hpp>

namespace tdc {

template<typename coder_t>
class LiteralEncoder: public Compressor {

public:
    inline static Meta meta() {
        Meta m("compressor", "encode", "Simply encodes the input's individual characters.");
        m.option("coder").templated<coder_t>();
        return m;
    }

    inline LiteralEncoder(Env&& env) : Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override final {
        auto iview = input.as_view();

        typename coder_t::Encoder coder(
            env().env_for_option("coder"), output, ViewLiterals(iview));

        for(uint8_t c : iview) {
            coder.encode(c, literal_r);
        }
    }

    inline virtual void decompress(Input& input, Output& output) override final {
        auto ostream = output.as_stream();        
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);

        while(!decoder.eof()) {
            ostream << decoder.template decode<uint8_t>(literal_r);
        }
    }
};

}

#endif
