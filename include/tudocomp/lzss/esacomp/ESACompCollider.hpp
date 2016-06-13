#ifndef _INCLUDED_ESACOMP_COLLIDER_HPP_
#define _INCLUDED_ESACOMP_COLLIDER_HPP_

#include <list>
#include <vector>
#include <sdsl/suffix_arrays.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {


/// Implements the "Collider" selection strategy for ESAComp.
///
/// TODO: Describe
class ESACompCollider {

private:
    struct Interval {
        size_t p, q, l;

        size_t other;
        bool deleted;

        std::list<size_t> collisions;
        size_t extent;

        Interval(size_t _p, size_t _q, size_t _l, size_t _other)
            : p(_p), q(_q), l(_l), other(_other), deleted(false), extent(0)
        {
        }

        bool set_deleted() {
            if(deleted) {
                return false;
            } else {
                deleted = true;
                return true;
            }
        }
    };

    Env *m_env;

public:
    inline ESACompCollider(Env& env) : m_env(&env) {
    }

    void factorize(const sdsl::csa_bitcompressed<>& sa,
                   const sdsl::int_vector<>& isa,
                   sdsl::int_vector<>& lcp,
                   size_t fact_min,
                   std::vector<LZSSFactor>& out_factors) {

        //induce intervals
        std::vector<Interval> intervals;
        for(size_t i = 1; i < sa.size(); i++) {
            if(lcp[i] >= fact_min) {
                size_t k = intervals.size();
                intervals.push_back(Interval(sa[i], sa[i-1], lcp[i], k+1));
                intervals.push_back(Interval(sa[i-1], sa[i], lcp[i], k));
            }
        }

        //process
        size_t count = intervals.size();
        while(count) {
            //debug output
            DLOG(INFO) << "Intervals (" << count << "):";
            {
                for(size_t i = 0; i < intervals.size(); i++) {
                    const Interval& it = intervals[i];
                    if(!it.deleted) {
                        DLOG(INFO) << "(" << i << "): [" << (it.p+1) << ", " << (it.q+1) << ", " << it.l << "] / (" << it.other << ")";
                    }
                }
            }

            //calculate collision extents
            for(size_t i = 0; i < intervals.size(); i++) {
                Interval& it = intervals[i];
                it.collisions.clear();
                it.extent = SIZE_MAX;

                if(it.deleted) continue;

                size_t lb = it.p;
                size_t rb = it.p + it.l;

                for(size_t k = 0; k < intervals.size(); k++) {
                    if(k == i) continue;
                    const Interval& kt = intervals[k];
                    if(kt.deleted) continue;
                    
                    //test possible collision cases
                    if(it.p >= kt.p && it.p <= kt.p + kt.l) {
                        //start of it is included in kt
                        it.collisions.push_back(k);
                        lb = std::min(lb, kt.p);
                    } else if(kt.p >= it.p && kt.p <= it.p + it.l) {
                        //start of kt is included in it
                        it.collisions.push_back(k);
                        rb = std::max(rb, kt.p + kt.l);
                    } else if(kt.p <= it.p && kt.p + kt.l >= it.p + it.l) {
                        //it is fully included in kt
                        it.collisions.push_back(k);
                        lb = std::min(lb, kt.p);
                        rb = std::max(rb, kt.p + kt.l);
                    } else if(it.p <= kt.p && it.p + it.l >= kt.p + kt.l) {
                        //kt is fully included in it
                        it.collisions.push_back(k);
                    }
                }

                if(it.collisions.empty()) {
                    it.extent = 0;
                } else {
                    it.extent = rb - lb;
                }
            }

            //find best
            size_t best = SIZE_MAX;
            size_t best_extent = SIZE_MAX;

            for(size_t i = 0; i < intervals.size(); i++) {
                Interval& it = intervals[i];
                if(!it.deleted) {
                    size_t extent = it.extent;

                    Interval& ot = intervals[it.other];
                    if(!ot.deleted) {
                        extent += ot.extent;
                    }

                    if(extent < best_extent || (extent == best_extent && it.l > intervals[best].l)) {
                        best = i;
                        best_extent = extent;
                    }
                }
            }

            //factorize
            #define DECREASE_INTERVAL_COUNT assert(count > 0); --count;

            if(best != SIZE_MAX) {
                Interval& it = intervals[best];
                DLOG(INFO) << "Pick: (" << best << ")";

                //factorize
                LZSSFactor fact(it.p, it.q, it.l);
                out_factors.push_back(fact); 

                //remove self
                it.set_deleted();
                DLOG(INFO) << "remove self (" << best << ")";
                DECREASE_INTERVAL_COUNT;

                //remove collisions
                for(auto x = it.collisions.begin(); x != it.collisions.end(); ++x) {
                    Interval& xt = intervals[*x];
                    if(xt.set_deleted()) {
                        DLOG(INFO) << "remove collision (" << (*x) << ")";
                        DECREASE_INTERVAL_COUNT;
                    }
                }

                //remove other
                {
                    size_t j = it.other;
                    Interval& jt = intervals[j];
                    if(jt.set_deleted()) {
                        DLOG(INFO) << "remove other (" << j << ")";
                        DECREASE_INTERVAL_COUNT;
                    }

                    //remove other full collisions
                    for(auto x = jt.collisions.begin(); x != jt.collisions.end(); ++x) {
                        Interval& xt = intervals[*x];
                        if(
                            (xt.p >= jt.p && xt.p + xt.l <= jt.p + jt.l) ||
                            (jt.p >= xt.p && jt.p + jt.l <= xt.p + xt.l)
                        ) {
                            if(xt.set_deleted()) {
                                DLOG(INFO) << "remove other full collision (" << (*x) << ")";
                                DECREASE_INTERVAL_COUNT;
                            }
                        }
                    }
                }
            } else {
                DLOG(INFO) << "didn't find anything good!?";
                //break;
            }

            #undef DECREASE_INTERVAL_COUNT
        } //while
    }
};

}}

#endif

