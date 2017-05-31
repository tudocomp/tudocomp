#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/def.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/lcpcomp/MaxLCPSuffixList.hpp>

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
private:
    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("lcpcomp_comp", "arrays");
        return m;
    }

    inline static ds::dsflags_t textds_flags() {
        return text_t::SA | text_t::ISA | text_t::LCP;
    }

    using Algorithm::Algorithm; //import constructor

    inline void factorize(text_t& text, size_t threshold, lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
        auto lcp = StatPhase::wrap("Construct Index Data Structures", [&] {
            text.require(text_t::SA | text_t::ISA | text_t::LCP);

            auto lcp = text.release_lcp();
            StatPhase::log("maxlcp", lcp.max_lcp());
            return lcp;
        });

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();

        if(lcp.max_lcp()+1 <= threshold) return; // nothing to factorize
        const size_t cand_length = lcp.max_lcp()+1-threshold;
        std::vector<index_t>* cand = new std::vector<index_t>[cand_length];

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
                + std::to_string(lcp.max_lcp()));

            for(size_t maxlcp = lcp.max_lcp(); maxlcp >= threshold; --maxlcp) {
                IF_STATS({
                    const index_fast_t maxlcpbits = bits_for(maxlcp-threshold);
                    if( ((maxlcpbits ^ (1UL<<(bits_for(maxlcpbits)-1))) == 0) && (( (maxlcp-threshold) ^ (1UL<<(maxlcpbits-1))) == 0)) { // only log at certain LCP values
                        phase.split(std::string{"Factors at max. LCP value "}
                            + std::to_string(maxlcp));
                        phase.log_stat("num factors", factors.size());
                    }
                })
                std::vector<index_t>& candcol = cand[maxlcp-threshold]; // select the vector specific to the LCP value
                for(size_t i = 0; i < candcol.size(); ++i) {
                    const index_t& index = candcol[i];
                    const auto& lcp_value = lcp[index];
                    if(lcp_value < maxlcp) { // if it got resized, we push it down
                        if(lcp_value < threshold) continue; // already erased
                        cand[lcp_value-threshold].push_back(index);
                        continue;
                    }
                    //generate factor
                    const index_fast_t pos_target = sa[index];
                    DCHECK_GT(index,0);
                    const index_fast_t pos_source = sa[index-1];
                    const index_fast_t factor_length = lcp[index];

                    factors.emplace_back(pos_target, pos_source, factor_length);

                    //erase suffixes on the replaced area
                    for(size_t k = 0; k < factor_length; ++k) {
                        lcp[isa[pos_target + k]] = 0;
                    }

                    const index_fast_t max_affect = std::min(factor_length, pos_target); //if pos_target is at the very beginning, we have less to scan
                    //correct intersecting entries
                    for(index_fast_t k = 0; k < max_affect; ++k) {
                        const index_fast_t pos_suffix = pos_target - k - 1; DCHECK_GE(pos_target,k+1);
                        const index_t& ind_suffix = isa[pos_suffix];
                        lcp[ind_suffix] = std::min<index_fast_t>(k+1, lcp[ind_suffix]);
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

