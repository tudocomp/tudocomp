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
    if(discard_null_terminator && text[n-1] == 0) {
        --n; // discard null terminator
    }

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
    Range flen_r(flen_min, flen_max);
    Range fdist_r(fdist_max);

    std::cerr << "flen_r = [" << flen_min << "," << flen_max << "]" << std::endl;
    std::cerr << "fdist_max = " << fdist_max << std::endl;

    // encode ranges
    coder.encode(n, len_r);
    coder.encode(flen_min, text_r);
    coder.encode(flen_max, text_r);
    coder.encode(fdist_max, text_r);

    size_t p = 0;
    for(size_t i = 0; i < factors.size(); i++) {
        const Factor& f = factors[i];

        if(factors[i].pos == p) coder.encode(false, bit_r);
        else {
            coder.encode(true, bit_r);
            coder.encode(factors[i].pos - p, fdist_r);
        }

        while(p < f.pos) {
            coder.encode(text[p++], literal_r);
        }

        // encode factor
        coder.encode(f.src, text_r);
        coder.encode(f.len, flen_r);

        p += f.len;
    }

    if(p < n) coder.encode(n - p, fdist_r);
    while(p < n)  {
        // encode symbol
        coder.encode(text[p++], literal_r);
    }

    // finalize
    coder.finalize();
}

template<typename coder_t, typename dcb_strategy_t>
inline void decode_text_internal(coder_t& decoder, std::ostream& outs) {
    // decode text range
    auto text_len = decoder.template decode<len_t>(len_r);
    Range text_r(text_len);

    // decode shortest and longest factor
    auto flen_min = decoder.template decode<len_t>(text_r);
    auto flen_max = decoder.template decode<len_t>(text_r);
    Range flen_r(flen_min, flen_max);

    // decode longest distance between factors
    auto fdist_max = decoder.template decode<len_t>(text_r);
    Range fdist_r(fdist_max);

    // init decode buffer
    DecodeBuffer<dcb_strategy_t> buffer(text_len);

    // decode
    while(!decoder.eof()) {
        len_t num;

        auto b = decoder.template decode<bool>(bit_r);
        if(b) decoder.template decode<len_t>(fdist_r);
        else num = 0;

        // decode characters
        while(num--) {
            auto c = decoder.template decode<uliteral_t>(literal_r);
            buffer.decode(c);
        }

        if(!decoder.eof()) {
            //decode factor
            auto src = decoder.template decode<len_t>(text_r);
            auto len = decoder.template decode<len_t>(flen_r);

            buffer.defact(src, len);
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
