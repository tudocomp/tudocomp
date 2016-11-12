#ifndef _INCLUDED_LZSS_LCP_COMPRESSOR_HPP_
#define _INCLUDED_LZSS_LCP_COMPRESSOR_HPP_

#include <algorithm>
#include <functional>
#include <vector>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>

#include <tudocomp/ds/TextDS.hpp>

namespace tdc {

/// Computes the LZ77 factorization of the input using its suffix array and
/// LCP table.
template<typename coder_t, typename len_t = uint32_t>
class LZSSLCPCompressor : public Compressor {

private:
    const TypeRange<len_t> len_r = TypeRange<len_t>();

    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("compressor", "lzss_lcp", "LZSS Factorization using LCP");
        m.option("coder").templated<coder_t>();
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
        text_t text(view, text_t::SA | text_t::ISA | text_t::LCP);
        env().end_stat_phase();

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();
        auto& lcp = text.require_lcp();

        // Factorize
        len_t len = text.size();
        lzss::FactorBuffer factors;

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
                factors.push_back(lzss::Factor(i, sa[p == p1 ? h1 : h2], p));

                i += p; //advance
            } else {
                ++i; //advance
            }
        }

        env().log_stat("threshold", fact_min);
        env().log_stat("factors", factors.size());
        env().end_stat_phase();

        // encode
        typename coder_t::Encoder coder(env().env_for_option("coder"),
            output, lzss::Literals<text_t>(text, factors));

        lzss::encode_text(coder, text, factors);
    }

    inline virtual void decompress(Input& input, Output& output) override {
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        auto outs = output.as_stream();

        lzss::decode_text(decoder, outs);
    }
};

}

#endif
