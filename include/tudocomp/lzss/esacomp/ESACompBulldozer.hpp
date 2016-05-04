#ifndef _INCLUDED_ESACOMP_BULLDOZER_HPP_
#define _INCLUDED_ESACOMP_BULLDOZER_HPP_

#include <vector>
#include <sdsl/suffix_arrays.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {


/// Implements the "Bulldozer" selection strategy for ESAComp.
///
/// TODO: Describe
class ESACompBulldozer {

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
    inline ESACompBulldozer() {
    }

    void factorize(const sdsl::csa_bitcompressed<>& sa,
                   const sdsl::int_vector<>& isa,
                   sdsl::int_vector<>& lcp,
                   size_t fact_min,
                   std::vector<LZSSFactor>& out_factors) {

        //
        size_t n = sa.size();

        //induce intervals
        std::vector<Interval> intervals;
        for(size_t i = 1; i < sa.size(); i++) {
            if(lcp[i] >= fact_min) {
                intervals.push_back(Interval{sa[i], sa[i-1], lcp[i]});
                intervals.push_back(Interval{sa[i-1], sa[i], lcp[i]});
            }
        }

        //sort
        std::sort(intervals.begin(), intervals.end(), IntervalComparator());

        //debug output
        /*DLOG(INFO) << "Intervals:";
        for(auto it = intervals.begin(); it != intervals.end(); ++it) {
            DLOG(INFO) << "[" << (it->p+1) << ", " << (it->q+1) << ", " << it->l << "]";
        }*/

        //marker
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

    }
};

}}

#endif

