#pragma once

#include <tudocomp/compressors/lz78/LZ78Coding.hpp>

#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/Decompressor.hpp>

namespace tdc {

template <typename coder_t>
class LZ78Decompressor : public Decompressor {
private:
    std::vector<lz78::factorid_t> indices;
    std::vector<uliteral_t> literals;

public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "lz78", "LZ78 decompressor.");
        m.param("coder", "The output decoder.")
            .strategy<coder_t>(TypeDesc("coder"), Meta::Default<BitCoder>());
        return m;
    }

    using Decompressor::Decompressor;

    virtual void decompress(Input& input, Output& output) override final {
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(config().sub_config("coder"), input);

        lz78::Decompressor decomp;
        uint64_t factor_count = 0;

        while (!decoder.eof()) {
            const lz78::factorid_t index = decoder.template decode<lz78::factorid_t>(Range(factor_count));
            const uliteral_t chr = decoder.template decode<uliteral_t>(literal_r);
            decomp.decompress(index, chr, out);
            factor_count++;
        }

        out.flush();
    }
};

}

