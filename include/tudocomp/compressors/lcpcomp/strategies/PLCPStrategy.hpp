#pragma once

#include <vector>

#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/ds/ArrayMaxHeap.hpp>

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
        Meta m("lcpcomp_strategy", "plcp");
        return m;
    }

    inline void factorize(text_t& text,
                   size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
		env().begin_stat_phase("Construct text ds");
		text.require(text_t::SA | text_t::ISA | text_t::PLCP);
		env().end_stat_phase();

        const auto& sa = text.require_sa();
        const auto& isa = text.require_isa();

        auto lcpp = text.release_plcp();
        auto lcp_datap = lcpp->relinquish();
        auto& plcp = *lcp_datap;

        const len_t n = sa.size();

        env().begin_stat_phase("Construct MaxLCPHeap");
 
		// compute number of max. entries of the heap
		len_t peaks = 0;
		for(len_t i = 0; i+1 < n; ++i) {
			if( (i == 0 || plcp[i] > plcp[i-1]) && plcp[i] >= threshold) {
				++peaks;
			}
		}
        // Construct heap
        ArrayMaxHeap<text_t::lcp_type::data_type> heap(plcp, plcp.size(), peaks);
		for(len_t i = 0; i+1 < n; ++i) {
			if( (i == 0 || plcp[i] > plcp[i-1]) && plcp[i] >= threshold) {
				heap.insert(i);	
			}
		}
        env().log_stat("entries", peaks);
        env().end_stat_phase();

        //Factorize
        env().begin_stat_phase("Process MaxLCPHeap");

		while(heap.size() > 0) {
			const len_t target_position = heap.get_max();
			const len_t factor_length = plcp[target_position]; 
			DCHECK_NE(isa[target_position], 0);
				DCHECK_LT(target_position+factor_length,n);
				const len_t source_position = sa[isa[target_position]-1];
				DCHECK_GE(factor_length, threshold);
				factors.emplace_back(target_position, source_position, factor_length);
				DCHECK([&] ()  {
						factors.sort();
						for(size_t i = 0; i+1 < factors.size(); ++i) {
							DCHECK_LE(factors[i].pos + factors[i].len, factors[i+1].pos);
						}
						return true;
						}());
				for(size_t k = 0; k < factor_length; ++k) {
					plcp[target_position + k] = 0;
	                heap.remove(target_position + k);
				}
				const len_t aff_length = std::min(factor_length+1, target_position); //! is setting factor_length to factor_length+1 the right thing?
				for(size_t k = 0; k < aff_length; ++k) {
					const len_t aff_position = target_position - k - 1;
					if(target_position < aff_position + plcp[aff_position]) {
						const len_t aff_lcp = target_position - aff_position;
						plcp[aff_position] = aff_lcp;
						if(heap.contains(aff_position)) {
							if(aff_lcp >= threshold) {
								heap.decrease_key(aff_position, aff_lcp);
							} else {
								heap.remove(aff_position);
							}
							
						}
					}
				}
				const len_t next_target_position = target_position+factor_length;
				if(next_target_position+1 < text.size() && plcp[next_target_position] >= threshold && !heap.contains(next_target_position)) {
					heap.insert(next_target_position);
				}
			}

        env().end_stat_phase();
    }
};

}}//ns

