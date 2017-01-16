#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/lcpcomp/MaxLCPSuffixList.hpp>

namespace tdc {
namespace lcpcomp {

/// Implements the original "Max LCP" selection strategy for LCPComp.
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
        Meta m("lcpcomp_strategy", "max_lcp");
        return m;
    }

    using Algorithm::Algorithm; //import constructor

    inline void factorize(text_t& text,
                   size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
		env().begin_stat_phase("Construct text ds");
		text.require(text_t::SA | text_t::ISA | text_t::LCP);
		env().end_stat_phase();

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();

        text.require_lcp();
        auto lcpp = text.release_lcp();
        auto lcp_datap = lcpp->relinquish();
        auto& lcp = *lcp_datap;

        env().begin_stat_phase("Construct MaxLCPSuffixList");
        MaxLCPSuffixList<text_t::lcp_type::data_type> list(lcp, threshold, lcpp->max_lcp());
        env().log_stat("entries", list.size());
        env().end_stat_phase();

        //Factorize
        env().begin_stat_phase("Process MaxLCPSuffixList");

        while(list.size() > 0) {
            //get suffix with longest LCP
            size_t m = list.get_max();

            //generate factor
            size_t fpos = sa[m];
            size_t fsrc = sa[m-1];
            size_t flen = lcp[m];

            factors.emplace_back(fpos, fsrc, flen);

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
                        size_t l = fpos - s;
                        if(l >= threshold) {
                            list.decrease_key(i, l);
                        } else {
                            list.remove(i);
                        }
                    }
                }
            }
        }

        env().end_stat_phase();
    }
};

}}

