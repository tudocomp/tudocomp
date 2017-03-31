#pragma once

#include <vector>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

namespace tdc {
namespace lcpcomp {

/// A very naive selection strategy for LCPComp.
///
/// TODO: Describe
class NaiveStrategy : public Algorithm {
private:
    typedef TextDS<> text_t;

public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("lcpcomp_comp", "naive");
        return m;
    }

    inline static ds::dsflags_t textds_flags() {
        return text_t::SA | text_t::ISA | text_t::LCP;
    }

    inline void factorize(text_t& text,
                   size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
        text.require(text_t::SA | text_t::ISA | text_t::LCP);

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();
        auto& lcp = text.require_lcp();

        //
        size_t n = sa.size();
        size_t i = 0;

        BitVector marked(n, 0);

        while(i+1 < n) { // we skip text position T[n] == 0
            size_t s = isa[i];
            size_t l = lcp[s];
            if(l >= threshold) {
                //check if any position is marked
                bool available = true;
                for(size_t k = 0; k < l; k++) {
                    if(marked[i + k]) {
                        available = false;
                        break;
                    }
                }

                if(available) {
                    //introduce factor
                    size_t src = sa[s - 1];
                    factors.emplace_back(i, src, l);

                    //mark source positions
                    for(size_t k = 0; k < l; k++) {
                        marked[src + k] = 1;
                    }

                    i += l;
                    continue;
                }
            }

            ++i;
        }
    }
};

}}

