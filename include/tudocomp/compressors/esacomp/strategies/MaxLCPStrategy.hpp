#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/esacomp/MaxLCPSuffixList.hpp>

namespace tdc {
namespace esacomp {

/// Implements the original "Max LCP" selection strategy for ESAComp.
///
/// This strategy constructs a deque containing suffix array entries sorted
/// by their LCP length. The entry with the maximum LCP is selected and
/// overlapping suffices are removed from the deque.
///
/// This was the original naive approach in "Textkompression mithilfe von
/// Enhanced Suffix Arrays" (BA thesis, Patrick Dinklage, 2015).
class MaxLCPStrategy : public Algorithm {
private:
    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("esacomp_strategy", "max_lcp");
        return m;
    }

    using Algorithm::Algorithm; //import constructor

    inline void factorize(text_t& text,
                   size_t threshold,
                   lzss::FactorBuffer& factors) {

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();

        text.require_lcp();
        auto _lcp = text.release_lcp();
        auto& lcp = _lcp->data();

        env().begin_stat_phase("Construct MaxLCPSuffixList");
        MaxLCPSuffixList<text_t::lcp_type> list(*_lcp, threshold);
        env().log_stat("entries", list.size());
        env().end_stat_phase();

        //Factorize
        env().begin_stat_phase("Process MaxLCPSuffixList");

        while(list.size() > 0) {
            //get suffix with longest LCP
            size_t m = list.first();

            //generate factor
            size_t fpos = sa[m];
            size_t fsrc = sa[m-1];
            size_t flen = lcp[m];

            factors.push_back(lzss::Factor(fpos, fsrc, flen));

            //remove overlapped entries
            for(size_t k = 0; k < flen; k++) {
                size_t i = isa[fpos + k];
                if(list.contains(i)) {
                    list.remove(i);
                }
            }

            //correct intersecting entries
            for(size_t k = 0; k < flen && fpos > k; k++) {
                size_t s = fpos - k - 1;
                size_t i = isa[s];
                if(list.contains(i)) {
                    if(s + lcp[i] > fpos) {
                        list.remove(i);

                        size_t l = fpos - s;
                        lcp[i] = l;
                        if(l >= threshold) {
                            list.insert(i);
                        }
                    }
                }
            }
        }

        env().end_stat_phase();
    }
};

}}

#pragma once
