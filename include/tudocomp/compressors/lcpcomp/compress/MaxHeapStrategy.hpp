#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/ArrayMaxHeap.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

#include <tudocomp_stat/StatPhase.hpp>

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
class MaxHeapStrategy : public Algorithm {
private:
    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("lcpcomp_comp", "heap");
        return m;
    }

    inline static ds::dsflags_t textds_flags() {
        return text_t::SA | text_t::ISA | text_t::LCP;
    }

    using Algorithm::Algorithm; //import constructor

    inline void factorize(text_t& text,
                   const size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
        StatPhase::wrap("Construct text ds", [&]{
            text.require(text_t::SA | text_t::ISA | text_t::LCP);
        });

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();
        auto lcp = text.release_lcp();

        auto heap = StatPhase::wrap("Construct MaxLCPHeap", [&]{
            // Count relevant LCP entries
            size_t heap_size = 0;
            for(size_t i = 1; i < lcp.size(); i++) {
                if(lcp[i] >= threshold) ++heap_size;
            }

            // Construct heap
            ArrayMaxHeap<text_t::lcp_type::data_type> heap(lcp, lcp.size(), heap_size);
            for(size_t i = 1; i < lcp.size(); i++) {
                if(lcp[i] >= threshold) heap.insert(i);
            }

            StatPhase::log("entries", heap.size());
            return heap;
        });

        //Factorize
        StatPhase::wrap("Process MaxLCPHeap", [&]{
            while(heap.size() > 0) {
                //get suffix with longest LCP
                size_t m = heap.get_max();

                //generate factor
                size_t fpos = sa[m];
                size_t fsrc = sa[m-1];
                size_t flen = lcp[m];

                factors.emplace_back(fpos, fsrc, flen);

                //remove overlapped entries
                for(size_t k = 0; k < flen; k++) {
                    heap.remove(isa[fpos + k]);
                }

                //correct intersecting entries
                for(size_t k = 0; k < flen && fpos > k; k++) {
                    size_t s = fpos - k - 1;
                    size_t i = isa[s];
                    if(heap.contains(i)) {
                        if(s + lcp[i] > fpos) {
                            size_t l = fpos - s;
                            if(l >= threshold) {
                                heap.decrease_key(i, l);
                            } else {
                                heap.remove(i);
                            }
                        }
                    }
                }
            }
        });
    }
};

}}

