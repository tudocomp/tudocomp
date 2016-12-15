#ifndef LAZYLIST_HPP
#define LAZYLIST_HPP

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/def.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/esacomp/MaxLCPSuffixList.hpp>

namespace tdc {
namespace esacomp {

/**
 * Creates arrays instead of an LCP-heap
 * Each array corresponds to one LCP value
 * We do not eagerly invoke decrease_key or erase.
 * Instead, we check for every element whether it got already deleted/its key got decreased
 * In the latter case, we push it down to the respective array
 */
class LazyListStrategy : public Algorithm {
private:
    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("esacomp_strategy", "lazy_list");
        return m;
    }

    using Algorithm::Algorithm; //import constructor

    inline void factorize(text_t& text, size_t threshold, lzss::FactorBuffer& factors) {

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();

        auto lcpp = text.release_lcp();
        auto& lcp = lcpp->data();

        env().log_stat("maxlcp", lcpp->max_lcp());
        if(lcpp->max_lcp()+1 <= threshold) return; // nothing to factorize
        const size_t cand_length = lcpp->max_lcp()+1-threshold;
        std::vector<len_t>* cand = new std::vector<len_t>[cand_length];
        env().begin_stat_phase("Fill candidates");
        for(size_t i = 1; i < sa.size(); ++i) {
            if(lcp[i] < threshold) continue;
            cand[lcp[i]-threshold].push_back(i);
        }
        env().log_stat("entries", [&] () { 
                size_t ret = 0; 
                for(size_t i = 0; i < cand_length; ++i) {  
                    ret += cand[i].size();
                }
                return ret; }());
        env().end_stat_phase();
        env().begin_stat_phase("Computing Factors");
        env().begin_stat_phase(std::string{"At MaxLCP Value "} + std::to_string(lcpp->max_lcp()) );
        for(size_t maxlcp = lcpp->max_lcp(); maxlcp >= threshold; --maxlcp) {
            if(tdc_stats(maxlcp < 4 || ((maxlcp ^ (1UL<<(bits_for(maxlcp)-1))) == 0))) { // only log at lcp-values that are a power of two or less than 4
                env().end_stat_phase();
                env().begin_stat_phase(std::string{"At MaxLCP Value "} + std::to_string(maxlcp) );
                env().log_stat("num factors", factors.size());
            }
            std::vector<len_t>& candcol = cand[maxlcp-threshold]; // select the vector specific to the LCP-value
            for(size_t i = 0; i < candcol.size(); ++i) {
                const len_t& index = candcol[i];
                const auto& lcp_value = lcp[index];
                if(lcp_value < maxlcp) { // if it got resized, we push it down
                    if(lcp_value < threshold) continue; // already erased
                    cand[lcp_value-threshold].push_back(index);
                    continue;
                }
                //generate factor
                const len_t pos_target = sa[index];
                DCHECK_GT(index,0);
                const len_t pos_source = sa[index-1];
                const len_t factor_length = lcp[index];

                factors.push_back(lzss::Factor { pos_target, pos_source, factor_length });

                //erase suffixes on the replaced area
                for(size_t k = 0; k < factor_length; ++k) {
                    lcp[isa[pos_target + k]] = 0;
                }

                const len_t max_affect = std::min(factor_length, pos_target); //if pos_target is at the very beginning, we have less to scan
                //correct intersecting entries
                for(len_t k = 0; k < max_affect; ++k) {
                    const len_t pos_suffix = pos_target - k - 1; DCHECK_GE(pos_target,k+1);
                    const len_t& ind_suffix = isa[pos_suffix];
                    lcp[ind_suffix] = std::min<len_t>(k+1, lcp[ind_suffix]);
                }

            }
            candcol.clear();
        }
        env().end_stat_phase();
        env().end_stat_phase();
        delete [] cand;
    }
};

}}

#endif /* LAZYLIST_HPP */

