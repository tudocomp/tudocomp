#pragma once

#include <tudocomp/Decompressor.hpp>
#include <tudocomp/compressors/lzss/DecompBackBuffer.hpp>

namespace tdc {
    template<typename lzss_coder_t>
    class LZSSDecompressor : public Decompressor {
    public:
        inline static Meta meta() {
            Meta m(Decompressor::type_desc(), "lzss",
                "Decompresses an LZSS factorized text.");
            m.param("decoder", "The input decoder.")
                .strategy<lzss_coder_t>(TypeDesc("lzss_coder"));
            return m;
        }

        using Decompressor::Decompressor;

    protected:
        template<typename dec_t>
        inline void decompress_internal(
            dec_t& decomp, Input& input, Output& output) {

            {
                auto decoder = lzss_coder_t(
                    config().sub_config("decoder")).decoder(input);

                decoder.decode(decomp);
            }

            auto outs = output.as_stream();
            decomp.write_to(outs);
        }

    public:
        inline virtual void decompress(Input& input, Output& output) override {
            lzss::DecompBackBuffer decomp;
            decompress_internal(decomp, input, output);
        }
    };
}

