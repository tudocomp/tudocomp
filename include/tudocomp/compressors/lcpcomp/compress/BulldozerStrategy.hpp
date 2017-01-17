#pragma once

#include <vector>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>

namespace tdc {
namespace lcpcomp {

/// Implements the "Bulldozer" selection strategy for LCPComp.
///
/// TODO: Describe
class BulldozerStrategy : public Algorithm {
private:
    typedef TextDS<> text_t;

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
    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lcpcomp_comp", "bulldozer");
        return m;
    }

    inline void factorize(text_t& text,
                   size_t threshold,
                   lzss::FactorBuffer& factors) {

		// Construct SA, ISA and LCP
		env().begin_stat_phase("Construct text ds");
		text.require(text_t::SA | text_t::ISA | text_t::LCP);
		env().end_stat_phase();

        auto& sa = text.require_sa();
        auto& lcp = text.require_lcp();

        //
        size_t n = sa.size();

        //induce intervals
        env().begin_stat_phase("Induce intervals");

        std::vector<Interval> intervals;
        for(size_t i = 1; i < sa.size(); i++) {
            if(lcp[i] >= threshold) {
                intervals.emplace_back(sa[i], sa[i-1], lcp[i]);
                intervals.emplace_back(sa[i-1], sa[i], lcp[i]);
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

                if(l >= threshold) {
                    factors.emplace_back(x->p, x->q, l);

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

