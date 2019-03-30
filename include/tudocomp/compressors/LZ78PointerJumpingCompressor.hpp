#pragma once

#include <tudocomp/compressors/lz_common/Lz78AlgoState.hpp>
#include <tudocomp/compressors/lz_pointer_jumping/BaseLZPointerJumpingCompressor.hpp>

namespace tdc {
namespace lz_pointer_jumping {
    using namespace lz_common;

    struct LZ78PointerJumping {
        template<typename encoder_t, typename dict_t, typename stats_t>
        using lz_state_t = Lz78AlgoState<encoder_t, dict_t, stats_t>;

        template<typename coder_t>
        using Decompressor = LZ78Decompressor<coder_t>;

        inline static char const* meta_name() {
            return "lz78_pj";
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
using LZ78PointerJumpingCompressor
    = lz_pointer_jumping::BaseLZPointerJumpingCompressor<lz_pointer_jumping::LZ78PointerJumping, coder_t, dict_t>;

}//ns
