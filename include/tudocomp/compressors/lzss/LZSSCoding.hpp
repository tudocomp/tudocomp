#pragma once

#include <cassert>

#include <tudocomp/Range.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/lzss/LZSSDecodeBackBuffer.hpp>
//#include <tudocomp/compressors/lzss/LZSSDecodeForwardChainBuffer.hpp>
//#include <tudocomp/compressors/lzss/LZSSDecodeForwardMultimapBuffer.hpp>
//#include <tudocomp/compressors/lzss/LZSSDecodeForwardListMapBuffer.hpp>
//#include <tudocomp/compressors/lzss/LZSSDecodeForwardQueueListBuffer.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lzss {

template<typename coder_t, typename text_t>
inline void encode_text(coder_t& coder,
                        const text_t& text,
                        const FactorBuffer& factors) {
    assert(factors.is_sorted());

    auto n = text.size();

    // determine longest and shortest factor
    auto flen_min = factors.shortest_factor();
    auto flen_max = factors.longest_factor();

    // determine longest distance between two factors
    size_t fdist_max = 0;
    {
        size_t p = 0;
        for(size_t i = 0; i < factors.size(); i++) {
            fdist_max = std::max(fdist_max, factors[i].pos - p);
            p = factors[i].pos + factors[i].len;
        }

        fdist_max = std::max(fdist_max, n - p);
    }

    // define ranges
    Range text_r(n);
    MinDistributedRange flen_r(flen_min, flen_max);
    Range fdist_r(fdist_max);

    // encode ranges
    coder.encode(n, len_r);
    coder.encode(flen_min, text_r);
    coder.encode(flen_max, text_r);
    coder.encode(fdist_max, text_r);

    // walk over factors
    size_t p = 0;
    for(size_t i = 0; i < factors.size(); i++) {
        const Factor& f = factors[i];

        if(factors[i].pos == p) {
            // cursor reached factor i, encode 0-bit
            coder.encode(false, bit_r);
        } else {
            // cursor did not yet reach factor i, encode 1-bit
            coder.encode(true, bit_r);

            // also encode amount of literals until factor i
            DCHECK_LE(p, factors[i].pos);
            coder.encode(factors[i].pos - p, fdist_r);
        }

        // encode literals until cursor reaches factor i
        while(p < f.pos) {
            coder.encode(text[p++], literal_r);
        }

        // encode factor
        DCHECK_LT(f.src + f.len, n);
        coder.encode(f.src, text_r);
        coder.encode(f.len, flen_r);

        p += f.len;
    }

    if(p < n) {
        coder.encode(true, bit_r);
        coder.encode(n - p, fdist_r);
    }

    while(p < n)  {
        // encode symbol
        coder.encode(text[p++], literal_r);
    }
}

template<typename coder_t, typename decode_buffer_t>
inline void decode_text(coder_t& decoder, std::ostream& outs) {
    // decode text range
    auto text_len = decoder.template decode<len_t>(len_r);
    Range text_r(text_len);

    // decode shortest and longest factor
    auto flen_min = decoder.template decode<len_t>(text_r);
    auto flen_max = decoder.template decode<len_t>(text_r);
    MinDistributedRange flen_r(flen_min, flen_max);

    // decode longest distance between factors
    auto fdist_max = decoder.template decode<len_t>(text_r);
    Range fdist_r(fdist_max);

    // init decode buffer
    decode_buffer_t buffer(text_len);

    // decode
    while(!decoder.eof()) {
        len_t num;

        auto b = decoder.template decode<bool>(bit_r);
        if(b) num = decoder.template decode<len_t>(fdist_r);
        else  num = 0;

        // decode characters
        while(num--) {
            auto c = decoder.template decode<uliteral_t>(literal_r);
            buffer.decode_literal(c);
        }

        if(!decoder.eof()) {
            //decode factor
            auto src = decoder.template decode<len_t>(text_r);
            auto len = decoder.template decode<len_t>(flen_r);

            buffer.decode_factor(src, len);
        }
    }

    // log stats
    StatPhase::log("longest_chain", buffer.longest_chain());

    // write decoded text
    buffer.write_to(outs);
}

// template<typename coder_t>
// inline void decode_text(coder_t& decoder, std::ostream& outs, bool allow_forward = false) {
//     if(allow_forward) {
// //        decode_text_internal<coder_t, DecodeForwardListMapBuffer>(decoder, outs);
//         //decode_text_internal<coder_t, SuccinctListBuffer>(decoder, outs);
//         decode_text_internal<coder_t, DecodeForwardQueueListBuffer>(decoder, outs);
//         //decode_text_internal<coder_t, DecodeForwardMultimapBuffer>(decoder, outs);
//     } else {
//         decode_text_internal<coder_t, DecodeBackBuffer>(decoder, outs);
//     }
// }

}} //ns

