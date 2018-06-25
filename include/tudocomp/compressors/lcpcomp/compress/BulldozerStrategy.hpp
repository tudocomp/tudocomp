#pragma once

#include <vector>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lcpcomp {

/// Implements the "Bulldozer" selection strategy for LCPComp.
///
/// TODO: Describe
class BulldozerStrategy : public Algorithm {
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
    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lcpcomp_comp", "bulldozer");
        return m;
    }

    inline static ds::dsflags_t textds_flags() {
        return ds::SA | ds::ISA | ds::LCP;
    }

    template<typename text_t, typename factorbuffer_t>
    inline void factorize(text_t& text, size_t threshold, factorbuffer_t& factors) {

		// Construct SA, ISA and LCP
        StatPhase::wrap("Construct text ds", [&]{
            text.require(text_t::SA | text_t::ISA | text_t::LCP);
        });

        auto& sa = text.require_sa();
        auto& lcp = text.require_lcp();

        //
        size_t n = sa.size();

        //induce intervals
        std::vector<Interval> intervals;
        StatPhase::wrap("Induce intervals", [&]{
            std::vector<Interval> intervals;
            for(size_t i = 1; i < sa.size(); i++) {
                if(lcp[i] >= threshold) {
                    intervals.emplace_back(sa[i], sa[i-1], lcp[i]);
                    intervals.emplace_back(sa[i-1], sa[i], lcp[i]);
                }
            }

            StatPhase::log("numIntervals", intervals.size());
        });

        //sort
        StatPhase::wrap("Sort intervals", [&]{
            std::sort(intervals.begin(), intervals.end(), IntervalComparator());
        });

        //debug output
        /*DLOG(INFO) << "Intervals:";
        for(auto it = intervals.begin(); it != intervals.end(); ++it) {
            DLOG(INFO) << "[" << (it->p+1) << ", " << (it->q+1) << ", " << it->l << "]";
        }*/

        //marker
        StatPhase::wrap("Process Intervals", [&]{
            BitVector marked(n);

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
        });
    }
};

}}

