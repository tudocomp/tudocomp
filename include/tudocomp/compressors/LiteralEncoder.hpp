#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/io.hpp>

namespace tdc {

template<typename coder_t>
class LiteralEncoder: public Compressor {

public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "encode",
            "Encodes the input symbol-wise.");
        m.param("coder", "The output encoder.")
            .strategy<coder_t>(TypeDesc("coder"));
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override final {
        auto iview = input.as_view();

        typename coder_t::Encoder coder(
            config().sub_config("coder"), output, ViewLiterals(iview));

        for(uint8_t c : iview) {
            coder.encode(c, literal_r);
        }
    }

    inline virtual void decompress(Input& input, Output& output) override final {
        auto ostream = output.as_stream();
        typename coder_t::Decoder decoder(config().sub_config("coder"), input);

        while(!decoder.eof()) {
            ostream << decoder.template decode<uint8_t>(literal_r);
        }
    }
};

}

