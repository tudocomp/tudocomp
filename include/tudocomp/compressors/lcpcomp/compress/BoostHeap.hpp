#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/ArrayMaxHeap.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <boost/heap/pairing_heap.hpp>

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
class BoostHeap : public Algorithm {
private:
    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("lcpcomp_comp", "bheap", "boost heaps");
        return m;
    }

    inline static ds::dsflags_t textds_flags() {
        return text_t::SA | text_t::ISA | text_t::LCP;
    }

    using Algorithm::Algorithm; //import constructor

    inline void factorize(text_t& text, const size_t threshold, lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
        StatPhase::wrap("Construct text ds", [&]{
            text.require(text_t::SA | text_t::ISA | text_t::LCP);
        });

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();

        text.require_lcp();
        auto lcp = text.release_lcp();

		struct LCPCompare {
			using lcp_t = decltype(lcp);
			using sa_t = decltype(sa); //`text_t::isa_type;
			lcp_t& m_lcp;
			sa_t& m_sa;
			LCPCompare(lcp_t& lcp_, sa_t& sa_) : m_lcp(lcp_), m_sa(sa_) {}

			bool operator()(const index_fast_t i, const index_fast_t j) const {
				if(m_lcp[i] == m_lcp[j]) return m_sa[i] > m_sa[j];
				return m_lcp[i] < m_lcp[j];
			}

		};

		LCPCompare comp(lcp,sa);

        StatPhase phase("Construct MaxLCPHeap");

		boost::heap::pairing_heap<index_t,boost::heap::compare<LCPCompare>> heap(comp);
		std::vector<decltype(heap)::handle_type> handles(lcp.size());

		handles[0].node_ = nullptr;
        for(size_t i = 1; i < lcp.size(); ++i) {
            if(lcp[i] >= threshold) handles[i] = heap.emplace(i);
			else handles[i].node_ = nullptr;
        }

        //Factorize
        phase.split("Process MaxLCPHeap");

        while(heap.size() > 0) {
            //get suffix with longest LCP
            const index_t& m = heap.top();

            //generate factor
            const index_fast_t fpos = sa[m];
            const index_fast_t fsrc = sa[m-1];
            const index_fast_t flen = lcp[m];

            factors.emplace_back(fpos, fsrc, flen);

            //remove overlapped entries
            for(size_t k = 0; k < flen; k++) {
                const index_fast_t pos = isa[fpos + k];
			    if(handles[pos].node_ == nullptr) continue;
                heap.erase(handles[pos]);
			    handles[pos].node_ = nullptr;
            }

            //correct intersecting entries
            for(size_t k = 0; k < flen && fpos > k; k++) {
                size_t s = fpos - k - 1;
                size_t i = isa[s];
			    if(handles[i].node_ != nullptr) {
                    if(s + lcp[i] > fpos) {
                        size_t l = fpos - s;
                        if(l >= threshold) {
						    lcp[i] = l;
						    heap.decrease(handles[i]);
                        } else {
						    heap.erase(handles[i]);
						    handles[i].node_ = nullptr;
                        }
                    }
                }
            }
        }
    }
};

}}

