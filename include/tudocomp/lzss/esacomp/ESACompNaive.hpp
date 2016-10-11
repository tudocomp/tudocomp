#ifndef _INCLUDED_ESACOMP_NAIVE_HPP_
#define _INCLUDED_ESACOMP_NAIVE_HPP_

#include <vector>

#include <tudocomp/ds/SuffixArray.hpp>
#include <tudocomp/ds/LCPArray.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/Algorithm.hpp>


namespace tdc {
namespace lzss {

/// A very naive selection strategy for ESAComp.
///
/// TODO: Describe
class ESACompNaive: Algorithm {

public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("esacomp_strategy", "bulldozer2");
        return m;
    }

        void factorize(TextDS<>& t,
                       size_t fact_min,
                       std::vector<LZSSFactor>& out_factors) {

        auto& sa = t.require_sa();
        auto& isa = t.require_isa();
        auto& lcp = t.require_lcp();

        //
        size_t n = sa.size();
        size_t i = 0;

        sdsl::bit_vector marked(n, 0);

        while(i < n) {
            size_t s = isa[i];
            size_t l = lcp[s];
            if(l >= fact_min) {
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
                    out_factors.push_back(LZSSFactor(i, src, l));

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

#endif

