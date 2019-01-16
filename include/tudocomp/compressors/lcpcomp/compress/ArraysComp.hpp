#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/def.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>
#include <tudocomp/compressors/lcpcomp/lcpcomp.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lcpcomp {

/**
 * Creates arrays instead of an LCP-heap
 * Each array corresponds to one LCP value
 * We do not eagerly invoke decrease_key or erase.
 * Instead, we check for every element whether it got already deleted/its key got decreased
 * In the latter case, we push it down to the respective array
 */
class ArraysComp : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(comp_strategy_type(), "arrays",
            "Uses arrays instead of maintaining a max heap");

        return m;
    }

    template<typename ds_t>
    inline static void construct_textds(ds_t& ds) {
        ds.template construct<
            ds::SUFFIX_ARRAY,
            ds::LCP_ARRAY,
            ds::INVERSE_SUFFIX_ARRAY>();
    }

    using Algorithm::Algorithm; //import constructor

    template<typename text_t, typename factorbuffer_t>
    inline void factorize(text_t& text, size_t threshold, factorbuffer_t& factors) {
        // get data structures
        auto& sa = text.template get<ds::SUFFIX_ARRAY>();
        auto& isa = text.template get<ds::INVERSE_SUFFIX_ARRAY>();

        const size_t max_lcp = text.template get_provider<ds::LCP_ARRAY>().max_lcp;
        StatPhase::log("maxlcp", max_lcp);
        auto lcp = text.template relinquish<ds::LCP_ARRAY>();

        if(max_lcp+1 <= threshold) return; // nothing to factorize
        const size_t cand_length = max_lcp+1-threshold;
        std::vector<len_compact_t>* cand = new std::vector<len_compact_t>[cand_length];

        StatPhase::wrap("Fill candidates", [&]{
            for(size_t i = 1; i < sa.size(); ++i) {
                if(lcp[i] < threshold) continue;
                cand[lcp[i]-threshold].push_back(i);
            }

            StatPhase::log("entries", [&] () {
                    size_t ret = 0;
                    for(size_t i = 0; i < cand_length; ++i) {
                        ret += cand[i].size();
                    }
                    return ret; }());
        });

        StatPhase::wrap("Compute Factors", [&]{
            StatPhase phase(std::string{"Factors at max. LCP value "}
                + to_string(max_lcp));

            for(size_t maxlcp = max_lcp; maxlcp >= threshold; --maxlcp) {
                IF_STATS({
                    const len_t maxlcpbits = bits_for(maxlcp-threshold);
                    if( ((maxlcpbits ^ (1UL<<(bits_for(maxlcpbits)-1))) == 0) && (( (maxlcp-threshold) ^ (1UL<<(maxlcpbits-1))) == 0)) { // only log at certain LCP values
                        phase.split(std::string{"Factors at max. LCP value "}
                            + to_string(maxlcp));
                        phase.log_stat("num factors", factors.size());
                    }
                })
                std::vector<len_compact_t>& candcol = cand[maxlcp-threshold]; // select the vector specific to the LCP value
                for(size_t i = 0; i < candcol.size(); ++i) {
                    const len_compact_t& index = candcol[i];
                    const auto& lcp_value = lcp[index];
                    if(lcp_value < maxlcp) { // if it got resized, we push it down
                        if(lcp_value < threshold) continue; // already erased
                        cand[lcp_value-threshold].push_back(index);
                        continue;
                    }
                    //generate factor
                    const len_t pos_target = sa[index];
                    DCHECK_GT(index,0u);
                    const len_t pos_source = sa[index-1];
                    const len_t factor_length = lcp[index];

                    factors.emplace_back(pos_target, pos_source, factor_length);

                    //erase suffixes on the replaced area
                    for(size_t k = 0; k < factor_length; ++k) {
                        lcp[isa[pos_target + k]] = 0;
                    }

                    const len_t max_affect = std::min(factor_length, pos_target); //if pos_target is at the very beginning, we have less to scan
                    //correct intersecting entries
                    for(len_t k = 0; k < max_affect; ++k) {
                        const len_t pos_suffix = pos_target - k - 1; DCHECK_GE(pos_target,k+1);
                        const len_t ind_suffix = isa[pos_suffix];
                        lcp[ind_suffix] = std::min<len_t>(k+1, lcp[ind_suffix]);
                    }

                }
                candcol.clear();
			    candcol.shrink_to_fit();
            }
        });
        delete [] cand;
    }
};

}}

