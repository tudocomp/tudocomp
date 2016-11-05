#ifndef _INCLUDED_LZSS_LCP_COMPRESSOR_HPP_
#define _INCLUDED_LZSS_LCP_COMPRESSOR_HPP_

#include <algorithm>
#include <functional>
#include <vector>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/ds/TextDS.hpp>

namespace tdc {

/// Computes the LZ77 factorization of the input using its suffix array and
/// LCP table.
template<typename C, typename len_t = uint32_t>
class LZSSLCPCompressor : public Compressor {

private:
    typedef TextDS<> text_t;

    struct lzfactors_t {
        len_t num;
        std::vector<len_t> pos, src, len;

        lzfactors_t() : num(0) {
        }

        inline void push_back(len_t _pos, len_t _src, len_t _len) {
            pos.push_back(_pos);
            src.push_back(_src);
            len.push_back(_len);
            num++;
        }
    };

public:
    inline static Meta meta() {
        Meta m("compressor", "lzss_lcp", "LZSS Factorization using LCP");
        m.option("coder").templated<C>();
        return m;
    }

    /// Default constructor (not supported).
    inline LZSSLCPCompressor() = delete;

    /// Construct the class with an environment.
    inline LZSSLCPCompressor(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
        auto view = input.as_view();

        // Construct text data structures
        env().begin_stat_phase("Construct SA, ISA and LCP");
        text_t t(view, text_t::SA | text_t::ISA | text_t::LCP);
        env().end_stat_phase();

        auto& sa = t.require_sa();
        auto& isa = t.require_isa();
        auto& lcp = t.require_lcp();

        // Factorize
        len_t len = t.size();
        lzfactors_t factors;

        env().begin_stat_phase("Factorize");
        len_t fact_min = 3; //factor threshold

        for(size_t i = 0; i < len;) {
            //get SA position for suffix i
            size_t h = isa[i];

            //search "upwards" in LCP array
            //include current, exclude last
            size_t p1 = lcp[h];
            ssize_t h1 = h - 1;
            if (p1 > 0) {
                while (h1 >= 0 && sa[h1] > sa[h]) {
                    p1 = std::min(p1, size_t(lcp[h1--]));
                }
            }

            //search "downwards" in LCP array
            //exclude current, include last
            size_t p2 = 0;
            size_t h2 = h + 1;
            if (h2 < len) {
                p2 = SSIZE_MAX;
                do {
                    p2 = std::min(p2, size_t(lcp[h2]));
                    if (sa[h2] < sa[h]) {
                        break;
                    }
                } while (++h2 < len);

                if (h2 >= len) {
                    p2 = 0;
                }
            }

            //select maximum
            size_t p = std::max(p1, p2);
            if (p >= fact_min) {
                // new factor
                factors.push_back(i, sa[p == p1 ? h1 : h2], p);

                i += p; //advance
            } else {
                ++i; //advance
            }
        }

        env().log_stat("threshold", fact_min);
        env().log_stat("factors", factors.num);
        env().end_stat_phase();

        // encode
        encode_text_lzss(t, factors, output);
    }

    inline virtual void decompress(Input& input, Output& output) override {
        //TODO
    }

private:
    inline void encode_text_lzss(
        const text_t& text,
        const lzfactors_t& factors,
        Output& output) {

        auto output_stream = output.as_stream();
        BitOStream out(output_stream);
        C coder(env().env_for_option("coder"), out);

        //define ranges
        TypeRange<len_t> r_len;
        BitRange r_bit;
        CharRange r_char;
        Range r_text(text.size());

        //encode text length
        coder.encode(text.size(), r_len);

        //TODO: factors must be sorted

        len_t p = 0;
        for(len_t i = 0; i < factors.num; i++) {
            while(p < factors.pos[i]) {
                uint8_t c = text[p++];

                // encode symbol
                coder.encode(0, r_bit);
                coder.encode(c, r_char);
            }

            // encode factor
            coder.encode(1, r_bit);
            coder.encode(factors.src[i], r_text);
            coder.encode(factors.len[i], r_text);

            p += factors.len[i];
        }

        while(p < text.size())  {
            uint8_t c = text[p++];

            // encode symbol
            coder.encode(0, r_bit);
            coder.encode(c, r_char);
        }

        // finalize
        coder.finalize();
    }
};

}

#endif
