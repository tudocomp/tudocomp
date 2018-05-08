#pragma once
#include <tudocomp/config.h>

#include <vector>
#include <iostream>
#include <fstream>

#include <boost/heap/pairing_heap.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/LCPSada.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lcpcomp {

struct PeakCandidate {
    len_t src;
    len_t dst;
    len_t lcp;
    len_t no;
    PeakCandidate(len_t _src, len_t _dst, len_t _lcp, len_t _no) : src(_src), dst(_dst), lcp(_lcp), no(_no) {}
    bool operator<(const PeakCandidate& o) const {
        DCHECK_NE(o.dst, this->dst);
        if(o.lcp == this->lcp) return this->dst > o.dst;
        return this->lcp < o.lcp;
    }
};


/** Strategy for generating the final factors
*  plcpcomp adds factors with add_factor, and calls sort
*  whenever it is sure that the next factors' target text position are always be greater
*  than all factors currently added. This is a good time to sort the already stored factors.
*  The method @factorize will finally produce the factors, and store them in @reflist.
*/
class RefLeftRAMStrategy {
        std::vector<PeakCandidate> m_poi_factors;

        struct poi_comparator {
            inline bool operator() (const PeakCandidate& poi1, const PeakCandidate& poi2) {
                return (poi1.dst < poi2.dst);
            }
        } poi_comp;

    public:

        void add_poi_factor(const PeakCandidate &factor) {
            return m_poi_factors.push_back(factor);
        }

        void factorize(lzss::FactorBufferRAM& reflist) {
            std::sort(m_poi_factors.begin(), m_poi_factors.end(), poi_comp);
            for(PeakCandidate poi : m_poi_factors) {
                reflist.emplace_back(poi.dst, poi.src, poi.lcp);
            }
        }
};

/**
* The actual PLCPcomp compressor: it searches for peaks, replaces them and adds the replacements to the RefStrategy
* @tparam RefStrategy container class that stores the factors (source,len). This class has to reconstruct the target.
*/
template<class RefStrategy,class plcp_type,class phi_type>
void compute_left_references(const size_t n, RefStrategy& refStrategy, plcp_type& pplcp, phi_type& phi, size_t threshold) {

    struct PhiPeakCandidateComp {
        bool operator() (const PeakCandidate& poi1, const PeakCandidate& poi2) {
            return (poi1.dst > poi2.dst);
        }
    };

    boost::heap::pairing_heap<PeakCandidate> heap;
    std::vector<typename boost::heap::pairing_heap<PeakCandidate>::handle_type> handles;

    std::priority_queue<PeakCandidate, std::vector<PeakCandidate>, PhiPeakCandidateComp> rightReferenceHeap;

    IF_STATS(len_t max_heap_size = 0);

    // std::stack<poi> pois; // text positions of interest, i.e., starting positions of factors we want to replace

    len_t rightRefPeaks = 0;

    len_t lastpos = 0;
    len_t lastpos_lcp = 0;
    PeakCandidate lastRightRef(0,0,0,0);
    for(len_t i = 0; i+1 < n; ++i) {
        while(pplcp.index() < i) pplcp.advance();
        const len_t plcp_i = pplcp();
        DCHECK_EQ(pplcp.index(), i);
        if(heap.empty()) {

            bool wasRightRef = false;
            while(!rightReferenceHeap.empty() && rightReferenceHeap.top().dst < i) rightReferenceHeap.pop();
            if(!rightReferenceHeap.empty()) {
                PeakCandidate nextRightRef = rightReferenceHeap.top();
                if(nextRightRef.dst == i && nextRightRef.lcp > plcp_i) {
                    rightRefPeaks++;
                    nextRightRef.no = handles.size();
                    handles.emplace_back(heap.push(nextRightRef));
                    lastpos = i;
                    lastpos_lcp = nextRightRef.lcp;
                    wasRightRef = true;
                }
            }

            if(plcp_i >= threshold) {
                // check if right-reference
                if(i < phi[i]) {
                    if(i >= lastRightRef.src + lastRightRef.lcp || plcp_i > lastRightRef.lcp - (i - lastRightRef.src)) {
                        lastRightRef = PeakCandidate(i, phi[i], plcp_i, 0);
                        rightReferenceHeap.push(lastRightRef);
                    }
                } else if(!wasRightRef) {
                    handles.emplace_back(heap.emplace(phi[i], i, plcp_i, handles.size()));
                    lastpos = i;
                    lastpos_lcp = plcp_i;
                }
            }
            continue;
        }

        if(i - lastpos >= lastpos_lcp || tdc_unlikely(i+1 == n)) { // condition that lastpos is a maximal peak
            IF_DEBUG(bool first = true);
            IF_STATS(max_heap_size = std::max<len_t>(max_heap_size, heap.size()));
            DCHECK_EQ(heap.size(), handles.size());
            while(!heap.empty()) {
                const PeakCandidate &top = heap.top();

                refStrategy.add_poi_factor(top); // add new factor with position top.dst and length top.lcp

                const len_t next_pos = top.dst; // store top, this is the current position that gets factorized
                IF_DEBUG(if(first) DCHECK_EQ(top.dst, lastpos); first = false;)

                {
                    len_t newlcp_peak = 0; // a new peak can emerge at top.dst+top.lcp
                    len_t newlcp_peak_src = 0;
                    bool peak_exists = false;
                    if(top.dst+top.lcp < i) {
                        for(len_t j = top.no+1; j < handles.size(); ++j) { // erase all right peaks that got substituted
                            if( handles[j].node_ == nullptr) continue;
                            const PeakCandidate poi = *(handles[j]);
                            DCHECK_LT(next_pos, poi.dst);
                            if(poi.dst < next_pos+top.lcp) {
                                heap.erase(handles[j]);
                                handles[j].node_ = nullptr;
                                if(poi.lcp + poi.dst > next_pos+top.lcp) {
                                    const len_t remaining_lcp = poi.lcp+poi.dst - (next_pos+top.lcp);
                                    DCHECK_NE(remaining_lcp,0);
                                    if(newlcp_peak != 0) DCHECK_LE(remaining_lcp, newlcp_peak);
                                    if(remaining_lcp > newlcp_peak) {
                                        newlcp_peak = remaining_lcp;
                                        newlcp_peak_src = poi.src + (poi.lcp - remaining_lcp);
                                    }
                                    //~ newlcp_peak = std::max(remaining_lcp, newlcp_peak);
                                }
                            } else if( poi.dst == next_pos+top.lcp) {
                                peak_exists=true;
                            } else {
                                break;    // only for performance
                            }
                        }
                    }
#ifdef DEBUG
                    if(peak_exists) {
                        for(len_t j = top.no+1; j < handles.size(); ++j) {
                            if( handles[j].node_ == nullptr) continue;
                            const PeakCandidate& poi = *(handles[j]);
                            if(poi.dst == next_pos+top.lcp) {
                                DCHECK_LE(newlcp_peak, poi.lcp);
                                break;
                            }
                        }
                    }
#endif
                    if(!peak_exists && newlcp_peak >= threshold) {
                        len_t j = top.no+1;
                        DCHECK(handles[j].node_ == nullptr);
                        handles[j] = heap.emplace(newlcp_peak_src, next_pos+top.lcp, newlcp_peak, j);
                    }

                }
                handles[top.no].node_ = nullptr;
                heap.pop(); // top now gets erased

                for(auto it = handles.rbegin(); it != handles.rend(); ++it) {
                    if( (*it).node_ == nullptr) continue;
                    PeakCandidate& poi = (*(*it));
                    if(poi.dst > next_pos)  continue;
                    const len_t newlcp = next_pos - poi.dst; // newlcp is actually the distance
                    if(newlcp < poi.lcp) {
                        if(newlcp < threshold) {
                            heap.erase(*it);
                            it->node_ = nullptr;
                        } else {
                            poi.lcp = newlcp;
                            heap.decrease(*it);
                        }
                    } else { // if the distance is too large then we are already too far away from the peaks that need to be truncated
                        break;
                    }
                }
            }
            //~ refStrategy.sort(); // the found references can now be sorted and appended to the already sorted list of references (appending preserves the ordering)
            handles.clear();
            --i;
            continue;

        }
        DCHECK_EQ(pplcp.index(), i);
        DCHECK_EQ(plcp_i, pplcp());
        if(plcp_i <= lastpos_lcp) continue;
        DCHECK_LE(threshold, plcp_i);

        bool wasRightRef = false;
        while(!rightReferenceHeap.empty() && rightReferenceHeap.top().dst < i) rightReferenceHeap.pop();
        if(!rightReferenceHeap.empty()) {
            PeakCandidate nextRightRef = rightReferenceHeap.top();
            if(nextRightRef.dst == i && nextRightRef.lcp > plcp_i) {
                rightRefPeaks++;
                nextRightRef.no = handles.size();
                handles.emplace_back(heap.push(nextRightRef));
                lastpos = i;
                lastpos_lcp = nextRightRef.lcp;
                wasRightRef = true;
            }
        }

        if(i < phi[i]) {
            if(i >= lastRightRef.src + lastRightRef.lcp || plcp_i > lastRightRef.lcp - (i - lastRightRef.src)) {
                lastRightRef = PeakCandidate(i, phi[i], plcp_i, 0);
                rightReferenceHeap.push(lastRightRef);
            }
        } else if (!wasRightRef) {
            handles.emplace_back(heap.emplace(phi[i], i, plcp_i, handles.size()));
            lastpos = i;
            //			DCHECK_EQ(plcp[lastpos], plcp_i);
            lastpos_lcp = plcp_i;
        }
    }

    IF_STATS(StatPhase::log("max heap size", max_heap_size));
}



/// A very naive selection strategy for LCPComp.
///
/// TODO: Describe
class PLCPLeftStrategy : public Algorithm {
    private:
        typedef TextDS<> text_t;

    public:
        using Algorithm::Algorithm;

        inline static Meta meta() {
            Meta m("lcpcomp_comp", "plcp_left", "compressor using PLCP array");
            return m;
        }

        /**
        *  Called by the LCPcompCompressor.
        *  The compressor works in RAM mode, so this method produces the factors in RAM.
        */
        inline void factorize(text_t& text, size_t threshold, lzss::FactorBufferRAM& refs) {
            StatPhase phase("Load Index DS");
            text.require(text_t::SA | text_t::PHI);

            const auto& sa = text.require_sa();
            const auto& phi = text.require_phi();

            RefLeftRAMStrategy refStrategy;
            LCPForwardIterator pplcp { (construct_plcp_bitvector(sa, text)) };
            phase.split("Compute References");
            compute_left_references(text.size(), refStrategy, pplcp, phi, threshold);
            phase.split("Factorize");
            refStrategy.factorize(refs);
        }

        inline static ds::dsflags_t textds_flags() {
            return text_t::SA | text_t::PHI;
        }
};

}
}//ns

