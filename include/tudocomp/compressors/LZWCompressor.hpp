#pragma once

#include <tudocomp/compressors/lz_common/LZWAlgoState.hpp>
#include <tudocomp/compressors/lz_common/BaseLZCompressor.hpp>

namespace tdc {
namespace lz_common {
    struct LZW {
        template<typename encoder_t, typename dict_t, typename stats_t>
        using lz_state_t = LZWAlgoState<encoder_t, dict_t, stats_t>;

        template<typename coder_t>
        using Decompressor = LZWDecompressor<coder_t>;

        inline static char const* meta_name() {
            return "lzw";
        }
        inline static char const* meta_desc() {
            return "Computes the Lempel-Ziv-Welch factorization of the input.";
        }
        inline static char const* stat_phase_name() {
            return "LZW Compression";
        }
    };
}

template <typename coder_t, typename dict_t>
using LZWCompressor = lz_common::BaseLZCompressor<lz_common::LZW, coder_t, dict_t>;

}//ns
