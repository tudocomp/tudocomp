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
class MaxHeapLeftOnlyStrategy : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("lcpcomp_comp", "heap_l");
        return m;
    }

    inline static ds::dsflags_t textds_flags() {
        return ds::SA | ds::ISA | ds::LCP;
    }

    using Algorithm::Algorithm; //import constructor

    template<typename text_t>
    inline void factorize(text_t& text,
                   const size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
        StatPhase::wrap("Construct text ds", [&]{
            text.require(text_t::SA | text_t::ISA | text_t::LCP);
        });

        const size_t n = text.size();
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
            ArrayMaxHeap<typename text_t::lcp_type::data_type> heap(lcp, lcp.size(), heap_size);
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
                size_t p = sa[m];
                size_t q = sa[m-1];
                size_t len = lcp[m];

                if(p < q) {
                    std::swap(p, q);
                }

                factors.emplace_back(p, q, len);

                // remove heap entries that would cause more replacements in p
                for(size_t k = 0; k < len; k++) {
                    // look at lexicographically smaller suffix
                    const size_t x = isa[p + k];
                    if( x > 0 &&
                        lcp[x] >= threshold &&
                        sa[x-1] < p+k) {

                        heap.remove(x);
                    }

                    // look at lexicographically larger suffix
                    if( x + 1 < n &&
                        lcp[x+1] >= threshold &&
                        sa[x+1] < p+k) {

                        heap.remove(x+1);
                    }
                }

                // correct left intersections into p
                for(size_t k = 0; k < len && p > k; k++) {
                    const size_t left = p - k - 1;
                    const size_t x = isa[left];
                    if(x > 0 && heap.contains(x)) {
                        if(sa[x-1] < left && left + lcp[x] > p) {
                            size_t l = p - left;
                            if(l >= threshold) {
                                heap.decrease_key(x, l);
                            } else {
                                heap.remove(x);
                            }
                        }
                    }

                    if(x+1 < n && heap.contains(x+1)) {
                        if(sa[x+1] < left && left + lcp[x+1] > p) {
                            size_t l = p - left;
                            if(l >= threshold) {
                                heap.decrease_key(x+1, l);
                            } else {
                                heap.remove(x+1);
                            }
                        }
                    }
                }
            }
        });
    }
};

}}

