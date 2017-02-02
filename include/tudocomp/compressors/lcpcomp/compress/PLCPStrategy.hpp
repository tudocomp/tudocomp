#pragma once
//#define Boost_FOUND 1
#include <tudocomp/config.h>

#include <vector>

#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/ds/ArrayMaxHeap.hpp>
#ifdef Boost_FOUND
#include <boost/heap/pairing_heap.hpp>
#endif

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
        Meta m("lcpcomp_comp", "plcp");
        return m;
    }

#ifdef Boost_FOUND
    inline void factorize(text_t& text,
                   size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
		env().begin_stat_phase("Construct text ds");
		text.require(text_t::SA | text_t::ISA | text_t::PLCP);
		env().end_stat_phase();
		env().begin_stat_phase("Search Peaks");

        const auto& sa = text.require_sa();
        const auto& isa = text.require_isa();

        auto lcpp = text.release_plcp();
        auto lcp_datap = lcpp->relinquish();
        auto& plcp = *lcp_datap;

        const len_t n = sa.size();

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
			const len_t plcp_i = plcp[i];
			if(heap.empty()) {
				if(plcp_i >= threshold) {
					handles.emplace_back(heap.emplace(i, plcp_i, handles.size()));
					lastpos = i;
					lastpos_lcp = plcp[lastpos];
				}
				continue;
			}
			if(i - lastpos >= lastpos_lcp || i+1 == n) {
				IF_DEBUG(bool first = true);
				IF_STATS(max_heap_size = std::max<len_t>(max_heap_size, heap.size()));
				DCHECK_EQ(heap.size(), handles.size());
				while(!heap.empty()) {
					const Poi& top = heap.top();
					const len_t source_position = sa[isa[top.pos]-1];
					factors.emplace_back(top.pos, source_position, top.lcp);
					const len_t next_pos = top.pos; // store top, this is the current position that gets factorized
					IF_DEBUG(if(first) DCHECK_EQ(top.pos, lastpos); first = false;)

					for(len_t i = top.no+1; i < handles.size(); ++i) { // erase all right peaks that got substituted
						if( handles[i].node_ == nullptr) continue;
						const Poi poi = *(handles[i]);
						DCHECK_LT(next_pos, poi.pos);
						if(poi.pos < next_pos+top.lcp) { 
							heap.erase(handles[i]);
							handles[i].node_ = nullptr;
							if(poi.lcp + poi.pos > next_pos+top.lcp) {
								bool has_overlapping = false; // number of peaks that cover the area of peak we are going to delete, but go on to the right -> we do not have to create a new peak if there is one
								for(len_t j = i+1; j < handles.size(); ++j) {
									if( handles[j].node_ == nullptr) continue;
									const Poi poi_cand = *(handles[j]);
									if(poi_cand.pos > poi.lcp + poi.pos) break;
									if(poi_cand.pos+poi_cand.lcp <= next_pos+top.lcp) continue;
									has_overlapping = true;
									break;
								}
								if(!has_overlapping) { // a new, but small peak emerged that was not covered by the peak poi
									const len_t remaining_lcp = poi.lcp+poi.pos - (next_pos+top.lcp);
									if(remaining_lcp >= threshold) {
										handles[i] = heap.emplace(next_pos+top.lcp, remaining_lcp, i);
									}
								}
							}
						}
						//else { break; } // !TODO
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
							//	continue; //!TODO
						}
						//break; // !TODO
					}
				}
				handles.clear();
				--i;
				continue;
			}
			DCHECK_EQ(plcp_i, plcp[i]);
			if(plcp_i <= lastpos_lcp) continue;
			DCHECK_LE(threshold, plcp[i]);
			handles.emplace_back(heap.emplace(i,plcp[i], handles.size()));
			lastpos = i;
			lastpos_lcp = plcp[lastpos];
		}
        IF_STATS(env().log_stat("max heap size", max_heap_size));
        env().end_stat_phase();
    }
#else//Boost_FOUND
    inline void factorize(text_t&, size_t, lzss::FactorBuffer& ) {
#warning "plcpcomp is a dummy without boost"
	}
#endif//Boost_FOUND

};

}}//ns

