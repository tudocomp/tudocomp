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

//TODO: sort multiple times

struct PeakCandidate {
    len_t src;
    len_t dst;
    len_t lcp;
    len_t no;
    PeakCandidate(len_t _src, len_t _dst, len_t _lcp, len_t _no) : src(_src), dst(_dst), lcp(_lcp), no(_no) {}
};

struct PeakLCPLess {
    bool operator()(const PeakCandidate& p1, const PeakCandidate& p2) const {
        return p1.lcp < p2.lcp;
    }
};

struct PeakDstGreater {
    bool operator()(const PeakCandidate& p1, const PeakCandidate& p2) const {
        return p1.dst > p2.dst;
    }
};

/**
* The actual PLCPcomp compressor: it searches for peaks, replaces them and adds the replacements to the RefStrategy
* @tparam RefStrategy container class that stores the factors (source,len). This class has to reconstruct the target.
*/
template<class factor_buffer_type,class plcp_type,class phi_type>
void compute_left_references(const size_t n, factor_buffer_type& factor_buffer, plcp_type& pplcp, phi_type& phi, size_t threshold) {

    typedef boost::heap::pairing_heap<PeakCandidate, boost::heap::compare<PeakLCPLess>> left_ref_heap_type;
    left_ref_heap_type left_ref_heap;
    std::vector<typename left_ref_heap_type::handle_type> left_ref_handles;

    std::priority_queue<PeakCandidate, std::vector<PeakCandidate>, PeakDstGreater> right_ref_heap;

    IF_STATS(len_t max_heap_size = 0);

    len_t lastdst = 0;
    len_t lastdst_lcp = threshold;
    PeakCandidate lastRightRef(0,0,0,0);
    for(len_t i = 0; i+1 < n; ++i) {
        while(pplcp.index() < i) pplcp.advance();
        const len_t plcp_i = pplcp();
        DCHECK_EQ(pplcp.index(), i);
        DCHECK_EQ(plcp_i, pplcp());

        if(!left_ref_heap.empty() && (i - lastdst >= lastdst_lcp || tdc_unlikely(i+1 == n))) { // condition that lastdst is a maximal peak
            IF_DEBUG(bool first = true);
            IF_STATS(max_heap_size = std::max<len_t>(max_heap_size, left_ref_heap.size()));
            DCHECK_EQ(left_ref_heap.size(), left_ref_handles.size());
            while(!left_ref_heap.empty()) {
                const PeakCandidate &top = left_ref_heap.top();

                factor_buffer.emplace_back(top.dst, top.src, top.lcp); // add new factor with position top.dst and length top.lcp

                const len_t top_dst = top.dst; // store top, this is the current position that gets factorized
                IF_DEBUG(if(first) DCHECK_EQ(top.dst, lastdst); first = false;)
                {
                    len_t newlcp_peak = 0; // a new peak can emerge at top.dst+top.lcp
                    len_t newlcp_peak_src = 0;
                    bool peak_exists = false;
                    if(top.dst+top.lcp < i) {
                        for(len_t j = top.no+1; j < left_ref_handles.size(); ++j) { // erase all right peaks that got substituted
                            if( left_ref_handles[j].node_ == nullptr) continue;
                            const PeakCandidate poi = *(left_ref_handles[j]);
                            DCHECK_LT(top_dst, poi.dst);
                            if(poi.dst < top_dst+top.lcp) {
                                left_ref_heap.erase(left_ref_handles[j]);
                                left_ref_handles[j].node_ = nullptr;
                                if(poi.lcp + poi.dst > top_dst+top.lcp) {
                                    const len_t remaining_lcp = poi.lcp+poi.dst - (top_dst+top.lcp);
                                    DCHECK_NE(remaining_lcp,0);
                                    if(newlcp_peak != 0) DCHECK_LE(remaining_lcp, newlcp_peak);
                                    if(remaining_lcp > newlcp_peak) {
                                        newlcp_peak = remaining_lcp;
                                        newlcp_peak_src = poi.src + (poi.lcp - remaining_lcp);
                                    }
                                    //~ newlcp_peak = std::max(remaining_lcp, newlcp_peak);
                                }
                            } else if( poi.dst == top_dst+top.lcp) {
                                peak_exists=true;
                            } else {
                                break;    // only for performance
                            }
                        }
                    }
#ifdef DEBUG
                    if(peak_exists) {
                        for(len_t j = top.no+1; j < left_ref_handles.size(); ++j) {
                            if( left_ref_handles[j].node_ == nullptr) continue;
                            const PeakCandidate& poi = *(left_ref_handles[j]);
                            if(poi.dst == top_dst+top.lcp) {
                                DCHECK_LE(newlcp_peak, poi.lcp);
                                break;
                            }
                        }
                    }
#endif
                    if(!peak_exists && newlcp_peak >= threshold) {
                        len_t j = top.no+1;
                        DCHECK(left_ref_handles[j].node_ == nullptr);
                        left_ref_handles[j] = left_ref_heap.emplace(newlcp_peak_src, top_dst+top.lcp, newlcp_peak, j);
                    }

                }
                left_ref_handles[top.no].node_ = nullptr;
                left_ref_heap.pop(); // top now gets erased

                for(auto it = left_ref_handles.rbegin(); it != left_ref_handles.rend(); ++it) {
                    if( (*it).node_ == nullptr) continue;
                    PeakCandidate& poi = (*(*it));
                    if(poi.dst > top_dst)  continue;
                    const len_t newlcp = top_dst - poi.dst; // newlcp is actually the distance
                    if(newlcp < poi.lcp) {
                        if(newlcp < threshold) {
                            left_ref_heap.erase(*it);
                            it->node_ = nullptr;
                        } else {
                            poi.lcp = newlcp;
                            left_ref_heap.decrease(*it);
                        }
                    } else { // if the distance is too large then we are already too far away from the peaks that need to be truncated
                        break;
                    }
                }
            }
            left_ref_handles.clear();
            
            lastdst = 0;
            lastdst_lcp = threshold;
        }


        bool i_points_right = i < phi[i];
        
        bool was_right_ref = false;
        while(!right_ref_heap.empty() && right_ref_heap.top().dst + right_ref_heap.top().lcp < i) right_ref_heap.pop();
        if(!right_ref_heap.empty()) {
            PeakCandidate next_right_ref = right_ref_heap.top();
            if(next_right_ref.dst <= i) {
                len_t offset = i - next_right_ref.dst;
                next_right_ref.dst += offset; // = i
                next_right_ref.src += offset;
                next_right_ref.lcp -= offset;
                
                if((i_points_right || next_right_ref.lcp > plcp_i) && next_right_ref.lcp > lastdst_lcp) {
                    next_right_ref.no = left_ref_handles.size();
                    left_ref_handles.emplace_back(left_ref_heap.push(next_right_ref));
                    lastdst = i;
                    lastdst_lcp = next_right_ref.lcp;
                    was_right_ref = true;
                }
            }
        }

        if(i_points_right) {
            if(plcp_i >= threshold) {
                if(i >= lastRightRef.src + lastRightRef.lcp || plcp_i > lastRightRef.lcp - (i - lastRightRef.src)) {
                    lastRightRef = PeakCandidate(i, phi[i], plcp_i, 0);
                    right_ref_heap.push(lastRightRef);
                }
            }
        } 
        else if(!was_right_ref && plcp_i > lastdst_lcp) {
            left_ref_handles.emplace_back(left_ref_heap.emplace(phi[i], i, plcp_i, left_ref_handles.size()));
            lastdst = i;
            lastdst_lcp = plcp_i;
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

            LCPForwardIterator pplcp { (construct_plcp_bitvector(sa, text)) };
            phase.split("Compute factors");
            compute_left_references(text.size(), refs, pplcp, phi, threshold);
            phase.split("Sort factors");
            refs.sort();
        }

        inline static ds::dsflags_t textds_flags() {
            return text_t::SA | text_t::PHI;
        }
};

}
}//ns

