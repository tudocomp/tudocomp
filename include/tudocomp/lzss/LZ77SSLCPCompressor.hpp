#ifndef _INCLUDED_LZ77SS_LCP_COMPRESSOR_HPP
#define _INCLUDED_LZ77SS_LCP_COMPRESSOR_HPP

#include <algorithm>
#include <functional>
#include <vector>

#include <tudocomp/ds/SuffixArray.hpp>
#include <tudocomp/ds/LCPArray.hpp>

#include <tudocomp/sdslex/int_vector_wrapper.hpp>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSCompressor.hpp>

namespace tudocomp {
namespace lzss {

/// Computes the LZ77 factorization of the input using its suffix array and
/// LCP table.
template<typename C>
class LZ77SSLCPCompressor : public LZSSCompressor<C> {

public:
    inline static Meta meta() {
        Meta m("compressor", "lz77ss_lcp", "LZ77 Factorization using LCP");
        m.option("coder").templated<C>();
        return m;
    }

    /// Default constructor (not supported).
    inline LZ77SSLCPCompressor() = delete;

    /// Construct the class with an environment.
    inline LZ77SSLCPCompressor(Env&& env):
        LZSSCompressor<C>(std::move(env)) {}

protected:
    /// \copydoc
    inline virtual bool pre_factorize(Input& input) override {
        return false;
    }

    /// \copydoc
    inline virtual LZSSCoderOpts coder_opts(Input& input) override {
        return LZSSCoderOpts(true, bitsFor(input.size()));
    }

    /// \copydoc
    inline virtual void factorize(Input& input) override {
        auto env = this->env();
        auto in = input.as_view();

        size_t len = in.size();
        const uint8_t* in_ptr = (const uint8_t*) in.data();

        //Construct SA
        env.begin_stat_phase("Construct SA and ISA");
        SuffixArray sa(in);
        env.end_stat_phase();

        auto isa = sa.isa;

        //Construct LCP
        env.begin_stat_phase("Construct LCP");
        LCPArray lcp(in, sa);
        env.end_stat_phase();

        //Factorize
        env.begin_stat_phase("Factorize");
        size_t fact_min = 3; //factor threshold

        size_t num_factors = 0;
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
                num_factors++;
                this->handle_fact(LZSSFactor(i, sa[p == p1 ? h1 : h2], p));
                i += p; //advance
            } else {
                this->handle_sym(in_ptr[i]);
                ++i; //advance
            }
        }

        env.log_stat("threshold", fact_min);
        env.log_stat("factors", num_factors);
        env.end_stat_phase();
    }
};

}}

#endif
