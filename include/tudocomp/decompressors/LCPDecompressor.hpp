#pragma once

#include <tudocomp/decompressors/LZSSDecompressor.hpp>
#include <tudocomp/compressors/lcpcomp/decompress/ScanDec.hpp>

namespace tdc {
    template<typename lzss_coder_t, typename dec_t = lcpcomp::ScanDec>
    class LCPDecompressor : public LZSSDecompressor<lzss_coder_t> {
    public:
        inline static Meta meta() {
            Meta m(Decompressor::type_desc(), "lcpcomp",
                "Decompresses an LZSS factorized text.");
            m.param("decoder", "The input decoder.")
                .strategy<lzss_coder_t>(TypeDesc("lzss_coder"));
            m.param("dec", "The strategy for decompression.")
                .strategy<dec_t>(lcpcomp::dec_strategy_type(),
                    Meta::Default<lcpcomp::ScanDec>());
            return m;
        }

        using LZSSDecompressor<lzss_coder_t>::LZSSDecompressor;

        inline virtual void decompress(Input& input, Output& output) override {
            dec_t decomp(this->config().sub_config("dec"));
            this->decompress_internal(decomp, input, output);
        }
    };
}

