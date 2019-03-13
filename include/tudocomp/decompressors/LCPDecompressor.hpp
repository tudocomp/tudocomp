#pragma once

#include <tudocomp/decompressors/LZSSDecompressor.hpp>

#ifdef SDSL_FOUND

    #include <tudocomp/compressors/lcpcomp/decompress/ScanDec.hpp>

    namespace tdc {
    namespace lcpcomp {
        using default_dec_t = ScanDec;
    }}

#else

    // SDSL not available, use CompactDec
    #include <tudocomp/compressors/lcpcomp/decompress/CompactDec.hpp>

    namespace tdc {
    namespace lcpcomp {
        using default_dec_t = CompactDec;
    }}

#endif

#include <tudocomp/compressors/lzss/LZSSCoder.hpp>

namespace tdc {
    template<typename lzss_coder_t, typename dec_t = lcpcomp::default_dec_t>
    class LCPDecompressor : public LZSSDecompressor<lzss_coder_t> {
    public:
        inline static Meta meta() {
            Meta m(Decompressor::type_desc(), "lcpcomp",
                "Decompresses an LZSS factorized text.");
            m.param("decoder", "The input decoder.")
                .strategy<lzss_coder_t>(lzss_bidirectional_coder_type());
            m.param("dec", "The strategy for decompression.")
                .strategy<dec_t>(lcpcomp::dec_strategy_type(),
                    Meta::Default<lcpcomp::default_dec_t>());
            return m;
        }

        using LZSSDecompressor<lzss_coder_t>::LZSSDecompressor;

        inline virtual void decompress(Input& input, Output& output) override {
            dec_t decomp(this->config().sub_config("dec"));
            this->decompress_internal(decomp, input, output);
        }
    };
}

