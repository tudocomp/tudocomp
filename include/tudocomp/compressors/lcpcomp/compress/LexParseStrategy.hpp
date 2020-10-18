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
class LexParseStrategy : public Algorithm {
public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m(comp_strategy_type(), "lexparse", "using lexparse [Prezza,Navarro]");
        return m;
    }

    template<typename ds_t>
    inline static void construct_textds(ds_t& ds) {
        ds.template construct<
            ds::SUFFIX_ARRAY,
            ds::PLCP_ARRAY,
            ds::INVERSE_SUFFIX_ARRAY>();
    }

    template<typename text_t, typename factorbuffer_t>
    inline void factorize(text_t& text, size_t threshold, factorbuffer_t& factors) {
	StatPhase::wrap("compute lexparse", [&]{
            const auto& sa = text.template get<ds::SUFFIX_ARRAY>();
            const auto& isa = text.template get<ds::INVERSE_SUFFIX_ARRAY>();

            auto plcp = text.template relinquish<ds::PLCP_ARRAY>();

            //
            const size_t n = sa.size();
		    len_t last_replacement_pos = 0;
		    for(len_t i = 0; i+1 < n; ) {
			    if(plcp[i] >= threshold) {
				    DCHECK_NE(isa[i], 0u);
				    const len_t& target_position = i;
				    const len_t factor_length = plcp[target_position];
				    DCHECK_LT(target_position+factor_length,n);
				    const len_t source_position = sa[isa[target_position]-1];
				    factors.emplace_back(i, source_position, factor_length);
				    i+= factor_length;
				    last_replacement_pos = i-1;
			    }
			    else {
				    ++i;
			    }
		    }
        });
    }
};

}}//ns

