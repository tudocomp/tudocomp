#ifndef _INCLUDED_LZSS_CODING_HPP_
#define _INCLUDED_LZSS_CODING_HPP_

#include <cassert>

#include <tudocomp/Range.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

#include <tudocomp/util/DecodeBuffer.hpp>

namespace tdc {
namespace lzss {

template<typename coder_t, typename text_t>
inline void encode_text(coder_t& coder,
                        const text_t& text,
                        const FactorBuffer& factors,
                        bool discard_null_terminator = true) {
    assert(factors.is_sorted());

    auto n = text.size();
    auto flen_max = factors.longest_factor();

    if(discard_null_terminator && text[n-1] == 0) {
        --n; // discard null terminator
    }

    // define ranges
    Range text_r(n);
    Range flen_r(flen_max);

    // encode text size and longest factor
    coder.encode(n, len_r);
    coder.encode(flen_max, text_r);

    size_t p = 0;
    for(size_t i = 0; i < factors.size(); i++) {
        const Factor& f = factors[i];

        while(p < f.pos) {
            auto c = text[p++];

            // encode symbol
            coder.encode(false, bit_r);
            coder.encode(c, literal_r);
        }

        // encode factor
        coder.encode(true, bit_r);
        coder.encode(f.src, text_r);
        coder.encode(f.len, flen_r);

        p += f.len;
    }

    while(p < n)  {
        auto c = text[p++];

        // encode symbol
        coder.encode(false, bit_r);
        coder.encode(c, literal_r);
    }

    // finalize
    coder.finalize();
}

template<typename coder_t, typename dcb_strategy_t>
inline void decode_text_internal(coder_t& decoder, std::ostream& outs) {
    // decode text range
    auto text_len = decoder.template decode<len_t>(len_r);
    Range text_r(text_len);

    // decode longest factor
    auto flen_max = decoder.template decode<len_t>(text_r);
    Range flen_r(flen_max);

    // init decode buffer
    DecodeBuffer<dcb_strategy_t> buffer(text_len);

    // decode
    while(!decoder.eof()) {
        bool is_factor = decoder.template decode<bool>(bit_r);
        if(is_factor) {
            auto src = decoder.template decode<len_t>(text_r);
            auto len = decoder.template decode<len_t>(flen_r);

            buffer.defact(src, len);
        } else {
            auto c = decoder.template decode<uliteral_t>(literal_r);
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
