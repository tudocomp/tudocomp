#ifndef _INCLUDED_ESACOMP_BULLDOZER_HPP_
#define _INCLUDED_ESACOMP_BULLDOZER_HPP_

#include <vector>

#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/Algorithm.hpp>


namespace tdc {
namespace lzss {


/// Implements the "Bulldozer" selection strategy for ESAComp.
///
/// TODO: Describe
class ESACompBulldozer: Algorithm {

private:
    struct Interval {
        size_t p, q, l;
    };

    struct IntervalComparator {
        bool operator()(const Interval& a, const Interval& b) {
            if(a.p != b.p) {
                return a.p < b.p;
            } else {
                return a.l > b.l;
            }
        }
    };

public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("esacomp_strategy", "bulldozer");
        return m;
    }

        void factorize(TextDS<>& t,
                       size_t fact_min,
                       std::vector<LZSSFactor>& out_factors) {

        auto& sa = t.require_sa();
        auto& lcp = t.require_lcp();

        //
        size_t n = sa.size();

        //induce intervals
        env().begin_stat_phase("Induce intervals");

        std::vector<Interval> intervals;
        for(size_t i = 1; i < sa.size(); i++) {
            if(lcp[i] >= fact_min) {
                intervals.push_back(Interval{sa[i], sa[i-1], lcp[i]});
                intervals.push_back(Interval{sa[i-1], sa[i], lcp[i]});
            }
        }

        env().log_stat("numIntervals", intervals.size());
        env().end_stat_phase();

        //sort
        env().begin_stat_phase("Sort intervals");
        std::sort(intervals.begin(), intervals.end(), IntervalComparator());
        env().end_stat_phase();

        //debug output
        /*DLOG(INFO) << "Intervals:";
        for(auto it = intervals.begin(); it != intervals.end(); ++it) {
            DLOG(INFO) << "[" << (it->p+1) << ", " << (it->q+1) << ", " << it->l << "]";
        }*/

        //marker
        env().begin_stat_phase("Process intervals");
        sdsl::bit_vector marked(n);

        auto x = intervals.begin();
        while(x != intervals.end()) {
            if(!marked[x->q]) {
                //find maximum amount of consecutive unmarked positions
                size_t l = 1;
                while(l < x->l && x->q + l < n && !marked[x->q + l]) {
                    ++l;
                }

                if(l >= fact_min) {
                    out_factors.push_back(LZSSFactor(x->p, x->q, l));

                    //mark source positions as "unreplaceable"
                    for(size_t k = 0; k < l; k++) {
                        marked[x->p + k] = 1;
                    }

                    //jump to next available interval
                    size_t p = x->p;
                    do {
                        ++x;
                    } while(x != intervals.end() && x->p < p + l);

                    continue;
                }
            }

            ++x;
        }

        env().end_stat_phase();
    }
};

}}

#endif

