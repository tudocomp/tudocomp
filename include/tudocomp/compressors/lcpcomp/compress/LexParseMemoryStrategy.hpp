#pragma once

#include <vector>

#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lcpcomp {

/**
 * a more memory-lightweight lex-parse compression strategy 
 * Runs in O(n * threshold) time instead of O(n) like lexparse. 
 *
 * Run with ./tdc -a 'lcpcomp(bi(binary,binary,binary), lexparsememory)' -0
 *
 */
class LexParseMemoryStrategy: public Algorithm {
public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m(comp_strategy_type(), "lexparsememory", "Memory-friendly variant of lexparse");
        return m;
    }

    template<typename ds_t>
    inline static void construct_textds(ds_t& ds) {
        ds.template construct< ds::PHI_ARRAY>();
    }

    template<typename text_t, typename factorbuffer_t>
    inline void factorize(text_t& text, size_t threshold, factorbuffer_t& factors) {
		StatPhase::wrap("Search Peaks", [&]{
            const auto& phi =  text.template get<ds::PHI_ARRAY>();

	    const auto& t = text.input;

            const size_t n = phi.size();

		    for(len_t i = 0; i+1 < n; ) {
			len_t factor_length = 0;
			const len_t source_position = phi[i];
			while(t[i+factor_length] == t[source_position+factor_length]) { 
			    ++factor_length; 
			};
			if(factor_length >= threshold) {
			    DCHECK_LT(i+factor_length,n);
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

