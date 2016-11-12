#ifndef _INCLUDED_LZSS_CODING_HPP_
#define _INCLUDED_LZSS_CODING_HPP_

#include <tudocomp/Range.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

namespace tdc {
namespace lzss {

template<typename coder_t, typename text_t>
inline void encode_text(coder_t& coder, const text_t& text, const FactorBuffer& factors) {
    // encode text size
    coder.encode(text.size(), size_r);

    // define ranges
    Range text_r(text.size());

    //TODO: factors must be sorted

    size_t p = 0;
    for(size_t i = 0; i < factors.size(); i++) {
        size_t fpos, fsrc, flen;
        std::tie(fpos, fsrc, flen) = factors[i];

        while(p < fpos) {
            uint8_t c = text[p++];

            // encode symbol
            coder.encode(0, bit_r);
            coder.encode(c, literal_r);
        }

        // encode factor
        coder.encode(1, bit_r);
        coder.encode(fsrc, text_r);
        coder.encode(flen, text_r);

        p += flen;
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

template<typename coder_t>
inline void decode_text(coder_t& decoder, std::ostream& outs) {
    // decode text range
    size_t text_len = decoder.template decode<size_t>(size_r);
    Range text_r(text_len);

    // init decode buffer
    // TODO: use different buffer type if "forward-factors" are allowed (e.g. esacomp)
    DecodeBuffer<DCBStrategyNone> buffer(text_len);

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

}} //ns

#endif
