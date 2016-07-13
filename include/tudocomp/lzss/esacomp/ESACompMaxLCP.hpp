#ifndef _INCLUDED_ESACOMP_MAX_LCP_HPP_
#define _INCLUDED_ESACOMP_MAX_LCP_HPP_

#include <vector>

#include <tudocomp/Env.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/util/MaxLCPSuffixList.hpp>
#include <tudocomp/Algorithm.hpp>

#include <tudocomp/ds/TextDS.hpp>

namespace tudocomp {
namespace lzss {

/// Implements the original "Max LCP" selection strategy for ESAComp.
///
/// This strategy constructs a deque containing suffix array entries sorted
/// by their LCP length. The entry with the maximum LCP is selected and
/// overlapping suffices are removed from the deque.
///
/// This was the original naive approach in "Textkompression mithilfe von
/// Enhanced Suffix Arrays" (BA thesis, Patrick Dinklage, 2015).
class ESACompMaxLCP: Algorithm {
    public:
        using Algorithm::Algorithm;

        inline static Meta meta() {
            Meta m("esacomp_strategy", "max_lcp");
            return m;
        }

        void factorize(TextDS& t,
                       size_t fact_min,
                       std::vector<LZSSFactor>& out_factors) {

            auto sa = t.require_sa();
            auto isa = t.require_isa();

            env().begin_stat_phase("Copy LCP array");
            auto _lcp = t.require_lcp();
            sdsl::int_vector<> lcp(_lcp.lcp);
            t.release_lcp();
            env().end_stat_phase();

            env().begin_stat_phase("Construct MaxLCPSuffixList");
            MaxLCPSuffixList<SuffixArray, sdsl::int_vector<>> list(sa, lcp, fact_min);
            env().log_stat("entries", list.size());
            env().end_stat_phase();

            //Factorize
            env().begin_stat_phase("Process MaxLCPSuffixList");

            while(list.size() > 0) {
                //get suffix with longest LCP
                size_t m = list.first();

                //generate factor
                LZSSFactor fact(sa[m], sa[m-1], lcp[m]);
                out_factors.push_back(fact);

                //remove overlapped entries
                for(size_t k = 0; k < fact.num; k++) {
                    size_t i = isa[fact.pos + k];
                    if(list.contains(i)) {
                        list.remove(i);
                    }
                }

                //correct intersecting entries
                for(size_t k = 0; k < fact.num && fact.pos > k; k++) {
                    size_t s = fact.pos - k - 1;
                    size_t i = isa[s];
                    if(list.contains(i)) {
                        if(s + lcp[i] > fact.pos) {
                            list.remove(i);

                            size_t l = fact.pos - s;
                            lcp[i] = l;
                            if(l >= fact_min) {
                                list.insert(i);
                            }
                        }
                    }
                }
            }

            env().end_stat_phase();
        }
};

}}

#endif

