#pragma once

#include <tudocomp/compressors/lzss/LZSSCoder.hpp>
#include <tudocomp/compressors/lzss/StreamingEncoder.hpp>
#include <tudocomp/compressors/lzss/StreamingDecoder.hpp>

namespace tdc {
namespace lzss {

/// \brief Stream coding strategy.
///
/// This strategy is used for streaming scenarios where the factors are not
/// buffered and nothing is known about the text in advance.
///
/// Naturally, this only allows for references to prior positions in the text.
template<typename ref_coder_t, typename len_coder_t, typename lit_coder_t>
class StreamingCoder : public LZSSCoder<ref_coder_t, len_coder_t, lit_coder_t> {
private:
    using super_t = LZSSCoder<ref_coder_t, len_coder_t, lit_coder_t>;

public:
    inline static Meta meta() {
        return super_t::meta(Meta("lzss_coder", "stream", "Streaming / online"));
    }

    using super_t::LZSSCoder;

    template<typename literals_t>
    inline auto encoder(Output& output, literals_t&& literals, Range len_r) {
        return super_t::template encoder<StreamingEncoder>(output, std::move(literals), len_r);
    }

    inline auto decoder(Input& input, Range len_r) {
        return super_t::template decoder<StreamingDecoder>(input, len_r);
    }
};

}}

