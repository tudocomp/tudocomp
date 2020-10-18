#pragma once

#include <tudocomp/compressors/lz_common/Lz78AlgoState.hpp>
#include <tudocomp/compressors/lz_common/BaseLZCompressor.hpp>

namespace tdc {
namespace lz_common {
    struct LZ78 {
        template<typename encoder_t, typename dict_t, typename stats_t>
        using lz_state_t = Lz78AlgoState<encoder_t, dict_t, stats_t>;

        template<typename coder_t>
        using Decompressor = LZ78Decompressor<coder_t>;

        inline static char const* meta_name() {
            return "lz78";
        }
        inline static char const* meta_desc() {
            return "Computes the Lempel-Ziv 78 factorization of the input.";
        }
        inline static char const* stat_phase_name() {
            return "Lz78 compression";
        }
    };
}

template <typename coder_t, typename dict_t>
using LZ78Compressor = lz_common::BaseLZCompressor<lz_common::LZ78, coder_t, dict_t>;

}//ns
