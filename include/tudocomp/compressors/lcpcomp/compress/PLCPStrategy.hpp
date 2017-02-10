#pragma once
#include <tudocomp/config.h>

#include <vector>

#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/ds/LCPSada.hpp>
#include <boost/heap/pairing_heap.hpp>

namespace tdc {
namespace lcpcomp {



/// A very naive selection strategy for LCPComp.
///
/// TODO: Describe
class PLCPStrategy : public Algorithm {
private:
    typedef TextDS<> text_t;

public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("lcpcomp_comp", "plcp", "compressor using PLCP array");
        return m;
    }

    inline void factorize(text_t& text,
                   size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
		env().begin_stat_phase("Construct index ds");
		text.require(text_t::SA | text_t::ISA);

        const auto& sa = text.require_sa();
        const auto& isa = text.require_isa();

		// TESTS
        // auto lcpp = text.release_plcp();
        // auto lcp_datap = lcpp->relinquish();
        // auto& plcp = *lcp_datap;
        //
		// {
		// 	typedef std::remove_reference<decltype(sa)>::type sa_t;
		// 	typedef DynamicIntVector phi_t;
		// 	phi_t phi { construct_phi_array<phi_t,sa_t>(sa) };
		// 	phi_algorithm(phi, text); 
		// 	for(size_t i = 0; i < plcp.size()-1; ++i) {
		// 		DCHECK_EQ(plcp[i], phi[i]);
		// 	}
		// }
        //
        //
		// auto lcp = construct_lcp_sada(env(), sa, text);
		// for(size_t i = 0; i < plcp.size()-1; ++i) {
		// 	DCHECK_EQ(plcp[i], lcp.plcp(i));
		// }
		// // for(size_t i = 0; i < plcp.size()-1; ++i) {
		// // 	DCHECK_EQ(plcp[i], lcp.naive_plcp(i));
		// // }
		// LCPForwardIterator pplcp { (construct_plcp_bitvector(env(), sa, text)) };
		// // for(size_t i = 0; i < plcp.size()-1; ++i) {
		// // 	DCHECK_EQ(plcp[i], pplcp());
		// // 	pplcp.advance();
		// // }

		LCPForwardIterator pplcp { (construct_plcp_bitvector(env(), sa, text)) };
        const len_t n = sa.size();

		env().end_stat_phase();
		env().begin_stat_phase("Search Peaks");

		struct Poi {
			len_t pos;
			len_t lcp;
			len_t no;
			Poi(len_t _pos, len_t _lcp, len_t _no) : pos(_pos), lcp(_lcp), no(_no) {}
			bool operator<(const Poi& o) const {
				DCHECK_NE(o.pos, this->pos);
				if(o.lcp == this->lcp) return this->pos > o.pos;
				return this->lcp < o.lcp;
			}
		};

		boost::heap::pairing_heap<Poi> heap;
		std::vector<boost::heap::pairing_heap<Poi>::handle_type> handles;

		IF_STATS(len_t max_heap_size = 0);

		// std::stack<poi> pois; // text positions of interest, i.e., starting positions of factors we want to replace

		len_t lastpos = 0;
		len_t lastpos_lcp = 0;
		for(len_t i = 0; i+1 < n; ++i) {
			while(pplcp.index() < i) pplcp.advance();
			const len_t plcp_i = pplcp(); DCHECK_EQ(pplcp.index(), i);
			if(heap.empty()) {
				if(plcp_i >= threshold) {
					handles.emplace_back(heap.emplace(i, plcp_i, handles.size()));
					lastpos = i;
					lastpos_lcp = plcp_i;
				}
				continue;
			}
			if(i - lastpos >= lastpos_lcp || tdc_unlikely(i+1 == n)) {
				IF_DEBUG(bool first = true);
				IF_STATS(max_heap_size = std::max<len_t>(max_heap_size, heap.size()));
				DCHECK_EQ(heap.size(), handles.size());
				while(!heap.empty()) {
					const Poi& top = heap.top();
					const len_t source_position = sa[isa[top.pos]-1];
					factors.emplace_back(top.pos, source_position, top.lcp);
					const len_t next_pos = top.pos; // store top, this is the current position that gets factorized
					IF_DEBUG(if(first) DCHECK_EQ(top.pos, lastpos); first = false;)

					{
						len_t newlcp_peak = 0; // a new peak can emerge at top.pos+top.lcp
						bool peak_exists = false;
						if(top.pos+top.lcp < i) 
						for(len_t j = top.no+1; j < handles.size(); ++j) { // erase all right peaks that got substituted
							if( handles[j].node_ == nullptr) continue;
							const Poi poi = *(handles[j]);
							DCHECK_LT(next_pos, poi.pos);
							if(poi.pos < next_pos+top.lcp) { 
								heap.erase(handles[j]);
								handles[j].node_ = nullptr;
								if(poi.lcp + poi.pos > next_pos+top.lcp) {
									const len_t remaining_lcp = poi.lcp+poi.pos - (next_pos+top.lcp);
									DCHECK_NE(remaining_lcp,0);
									if(newlcp_peak != 0) DCHECK_LE(remaining_lcp, newlcp_peak); 
									newlcp_peak = std::max(remaining_lcp, newlcp_peak);
								}
							} else if( poi.pos == next_pos+top.lcp) { peak_exists=true; }
							else { break; }  // only for performance
						}
#ifdef DEBUG
						if(peak_exists) {  //TODO: DEBUG
							for(len_t j = top.no+1; j < handles.size(); ++j) { 
								if( handles[j].node_ == nullptr) continue;
								const Poi& poi = *(handles[j]);
								if(poi.pos == next_pos+top.lcp) {
									DCHECK_LE(newlcp_peak, poi.lcp);
									break;
								}
							}
						}
#endif
						if(!peak_exists && newlcp_peak >= threshold) {
							len_t j = top.no+1;
							DCHECK(handles[j].node_ == nullptr);
							handles[j] = heap.emplace(next_pos+top.lcp, newlcp_peak, j);
						}
						
					}
					handles[top.no].node_ = nullptr;
					heap.pop(); // top now gets erased

					for(auto it = handles.rbegin(); it != handles.rend(); ++it) {
						if( (*it).node_ == nullptr) continue;
						Poi& poi = (*(*it));
						if(poi.pos > next_pos)  continue;
						const len_t newlcp = next_pos - poi.pos;
						if(newlcp < poi.lcp) {
							if(newlcp < threshold) {
								heap.erase(*it);
								it->node_ = nullptr;
							} else {
								poi.lcp = newlcp;
								heap.decrease(*it);

							}
						} else {
							break;
						}
					}
				}
				handles.clear();
				--i;
				continue;
			}
			DCHECK_EQ(pplcp.index(), i);
			DCHECK_EQ(plcp_i, pplcp());
			if(plcp_i <= lastpos_lcp) continue;
			DCHECK_LE(threshold, plcp_i);
			handles.emplace_back(heap.emplace(i,plcp_i, handles.size()));
			lastpos = i;
//			DCHECK_EQ(plcp[lastpos], plcp_i);
			lastpos_lcp = plcp_i;
		}
        IF_STATS(env().log_stat("max heap size", max_heap_size));
        env().end_stat_phase();
    }

};

}}//ns

