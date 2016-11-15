#ifndef _INCLUDED_LZSS_CODING_HPP_
#define _INCLUDED_LZSS_CODING_HPP_

#include <cassert>

#include <tudocomp/Range.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

#include <tudocomp/util/DecodeBuffer.hpp>

namespace tdc {
namespace lzss {

template<typename coder_t, typename text_t>
inline void encode_text(coder_t& coder, const text_t& text, const FactorBuffer& factors) {
    assert(factors.is_sorted());

    // encode text size
    coder.encode(text.size(), size_r);

    // define ranges
    Range text_r(text.size());

    size_t p = 0;
    for(size_t i = 0; i < factors.size(); i++) {
        const Factor& f = factors[i];

        while(p < f.pos) {
            uint8_t c = text[p++];

            // encode symbol
            coder.encode(0, bit_r);
            coder.encode(c, literal_r);
        }

        // encode factor
        coder.encode(1, bit_r);
        coder.encode(f.src, text_r);
        coder.encode(f.len, text_r);

        p += f.len;
    }

    while(p < text.size())  {
        uint8_t c = text[p++];

        // encode symbol
        coder.encode(0, bit_r);
        coder.encode(c, literal_r);
    }

    // finalize
    coder.finalize();
}

template<typename coder_t, typename dcb_strategy_t>
inline void decode_text_internal(coder_t& decoder, std::ostream& outs) {
    // decode text range
    size_t text_len = decoder.template decode<size_t>(size_r);
    Range text_r(text_len);

    // init decode buffer
    DecodeBuffer<dcb_strategy_t> buffer(text_len);

    // decode
    while(!decoder.eof()) {
        bool is_factor = decoder.template decode<bool>(bit_r);
        if(is_factor) {
            size_t src = decoder.template decode<size_t>(text_r);
            size_t len = decoder.template decode<size_t>(text_r);

            buffer.defact(src, len);
        } else {
            uint8_t c = decoder.template decode<uint8_t>(literal_r);
            buffer.decode(c);
        }
    }

    // write decoded text
    buffer.write_to(outs);
}

template<typename coder_t>
inline void decode_text(coder_t& decoder, std::ostream& outs, bool allow_forward = false) {
    if(allow_forward) {
        decode_text_internal<coder_t, DCBStrategyRetargetArray>(decoder, outs);
    } else {
        decode_text_internal<coder_t, DCBStrategyNone>(decoder, outs);
    }
}

}} //ns

#endif