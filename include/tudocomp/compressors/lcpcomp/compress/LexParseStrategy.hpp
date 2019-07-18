#pragma once

#include <vector>

#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lcpcomp {

/// A very naive selection strategy for LCPComp.
///
/// TODO: Describe
class LexParseStrategy: public Algorithm {
public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m(comp_strategy_type(), "lexparse", "Greedy parse in text order, referring to lexicographically next-smaller suffix. Name coined by Nicola Prezza");
        return m;
    }

    template<typename ds_t>
    inline static void construct_textds(ds_t& ds) {
        ds.template construct< ds::PHI_ARRAY, ds::PLCP_ARRAY>();
    }

    template<typename text_t, typename factorbuffer_t>
    inline void factorize(text_t& text, size_t threshold, factorbuffer_t& factors) {
		StatPhase::wrap("Search Peaks", [&]{
            const auto& phi =  text.template get<ds::PHI_ARRAY>();
            auto plcp = text.template relinquish<ds::PLCP_ARRAY>();

            const size_t n = phi.size();

		    for(len_t i = 0; i+1 < n; ) {
			if(plcp[i] >= threshold) {
			    const len_t factor_length = plcp[i];
			    DCHECK_LT(i+factor_length,n);
			    const len_t source_position = phi[i];
			    factors.emplace_back(i, source_position, factor_length);
			    i+= factor_length;
			} else {
			    ++i;
			}
		    }
        });
    }
};

}}//ns

