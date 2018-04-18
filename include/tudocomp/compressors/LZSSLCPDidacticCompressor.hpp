#pragma once

#include <sstream>

#include <tudocomp/compressors/LZSSLCPCompressor.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>

namespace tdc {

/// Computes the LZ77 factorization of the input using its suffix array and
/// LCP table, produces a didactic output.
template<typename text_t = TextDS<>>
class LZSSLCPDidacticCompressor : public LZSSLCPCompressor<ASCIICoder, text_t> {
public:
    inline static Meta meta() {
        Meta m("compressor", "lzss_lcp_didactic",
            "LZSS Factorization using LCP with didactical output");
        m.option("textds").templated<text_t, TextDS<>>("textds");
        m.option("threshold").dynamic(3);
        //m.uses_textds<text_t>(text_t::SA | text_t::ISA | text_t::LCP);
        m.input_restrictions(io::InputRestrictions({0, '{', '}'}, true));
        return m;
    }

    using LZSSLCPCompressor<ASCIICoder, text_t>::LZSSLCPCompressor;

    inline virtual void encode(
        Output& output,
        const text_t& text,
        const lzss::FactorBuffer& factors) override
    {
        auto os = output.as_stream();

        size_t p = 0;
        for(const auto& f : factors) {
            while(p < f.pos) {
                os << text[p++];
            }

            os << '{' << (f.src+1) << ',' << f.len << '}';
            p += size_t(f.len);
        }

        while(p < text.size()) {
            os << text[p++];
        }
    }

    inline virtual void decompress(Input& input, Output& output) override {
        std::vector<uliteral_t> text;

        {
            auto is = input.as_stream();
            
            while(!is.eof()) {
                uliteral_t c = is.get();
                if(c == '{') {
                    // factor
                    std::stringstream spos, slen;

                    // read source position
                    do {
                        c = is.get();
                        if(c != ',') spos << c;
                    } while(c != ',');
                    
                    // read length
                    do {
                        c = is.get();
                        if(c != '}') slen << c;
                    } while(c != '}');

                    // parse factor
                    size_t fpos, flen;
                    spos >> fpos; --fpos;
                    slen >> flen;

                    // copy
                    for(size_t k = 0; k < flen; k++) {
                        text.emplace_back(text[fpos+k]);
                    }
                } else {
                    // other character
                    text.emplace_back(c);    
                }
            }
        }

        auto os = output.as_stream();
        os.write((const char*)text.data(), text.size());
    }
};

}

