#pragma once

#include <cassert>

#include <tudocomp/Range.hpp>
#include <tudocomp/compressors/lzss/FactorBuffer.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lzss {

template<typename coder_t, typename text_t, typename factor_t>
inline void encode_text(coder_t& coder, text_t& text, const factor_t& factors) {
    assert(factors.is_sorted());

    const auto n = text.size();

    // determine longest and shortest factor
    const auto flen_min = factors.shortest_factor();
    const auto flen_max = factors.longest_factor();

    // determine longest distance between two factors
    size_t fdist_max = 0;
    {
        size_t p = 0;
        for(auto it = factors.begin(); it != factors.end(); ++it) {
            const Factor& factor = *it;
            fdist_max = std::max(fdist_max, factor.pos - p);
            p = factor.pos + factor.len;
        }

        fdist_max = std::max(fdist_max, n - p);
    }

    // define ranges
    const Range text_r(n);
    const MinDistributedRange flen_r(flen_min, flen_max); //! range for factor lengths
    const Range fdist_r(fdist_max);

    // encode ranges
    coder.encode(n, len_r);
    coder.encode(flen_min, text_r);
    coder.encode(flen_max, text_r);
    coder.encode(fdist_max, text_r);

    // walk over factors
    size_t p = 0; //! current text position
    for(auto it = factors.begin(); it != factors.end(); ++it) {
        const Factor& factor = *it;

        if(factor.pos == p) {
            // cursor reached factor i, encode 0-bit
            coder.encode(false, bit_r);
        } else {
            // cursor did not yet reach factor i, encode 1-bit
            coder.encode(true, bit_r);

            // also encode amount of literals until factor i
            DCHECK_LE(p, factor.pos);
            DCHECK_LE(factor.pos - p, fdist_max); //distance cannot be larger than maximum distance
            coder.encode(factor.pos - p, fdist_r);
        }

        // encode literals until cursor reaches factor i
        while(p < factor.pos) {
            coder.encode(text[p++], literal_r);
        }

        // encode factor
        DCHECK_LT(factor.src + factor.len, n);
        coder.encode(factor.src, text_r);
        coder.encode(factor.len, flen_r);

        p += size_t(factor.len);
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

}} //ns

