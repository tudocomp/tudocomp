#ifndef _INCLUDED_ESACOMP_COLLIDER_HPP_
#define _INCLUDED_ESACOMP_COLLIDER_HPP_

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

        Interval(size_t _p, size_t _q, size_t _l, size_t _other)
            : p(_p), q(_q), l(_l), other(_other), deleted(false)
        {
        }
    };

public:
    inline ESACompCollider() {
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

            //save best pick
            struct {
                size_t index, length;
                size_t extent;
                std::list<size_t> collisions;
            } best;

            best.index = SIZE_MAX;
            best.extent = SIZE_MAX;

            //find interval with smallest collision size
            for(size_t i = 0; i < intervals.size(); i++) {
                const Interval& it = intervals[i];
                if(it.deleted) continue;

                size_t lb = it.p;
                size_t rb = it.p + it.l;
                std::list<size_t> collisions;

                for(size_t k = 0; k < intervals.size(); k++) {
                    if(k == i) continue;
                    const Interval& kt = intervals[k];
                    if(kt.deleted) continue;
                    
                    //test possible collision cases
                    if(it.p >= kt.p && it.p <= kt.p + kt.l) {
                        //start of it is included in kt
                        collisions.push_back(k);
                        lb = std::min(lb, kt.p);
                    } else if(kt.p >= it.p && kt.p <= it.p + it.l) {
                        //start of kt is included in it
                        collisions.push_back(k);
                        rb = std::max(rb, kt.p + kt.l);
                    } else if(kt.p <= it.p && kt.p + kt.l >= it.p + it.l) {
                        //it is fully included in kt
                        collisions.push_back(k);
                        lb = std::min(lb, kt.p);
                        rb = std::max(rb, kt.p + kt.l);
                    } else if(it.p <= kt.p && it.p + it.l >= kt.p + kt.l) {
                        //kt is fully included in it
                        collisions.push_back(k);
                    }
                }

                if(collisions.empty()) {
                    best.extent = 0;
                    best.index = i;
                    best.length = it.l;
                    best.collisions.clear();
                    break;
                } else {
                    size_t extent = rb - lb;
                    DLOG(INFO) << "collision extent of (" << i << ") is: " << extent;

                    if(extent < best.extent || (extent == best.extent && it.l > best.length)) {
                        best.extent = extent;
                        best.index = i;
                        best.length = it.l;
                        best.collisions = collisions;
                    }
                }
            }

            //factorize best
            if(best.index != SIZE_MAX) {
                Interval& it = intervals[best.index];
                DLOG(INFO) << "Pick: (" << best.index << ")";

                //factorize
                LZSSFactor fact(it.p, it.q, it.l);
                out_factors.push_back(fact); 

                //remove self
                it.deleted = true;
                assert(count > 0);
                --count; 

                //remove other and subintervals of other
                {
                    size_t j = it.other;
                    size_t jl;
                    do {
                        Interval& jt = intervals[j];

                        if(!jt.deleted) {
                            DLOG(INFO) << "remove other (" << j << ")";
                            jt.deleted = true;
                            assert(count > 0);
                            --count;
                        }

                        jl = jt.l;
                        j += 2;
                    } while(j < intervals.size() && intervals[j].l < jl);
                }

                //remove collisions
                for(auto x = best.collisions.begin(); x != best.collisions.end(); ++x) {
                    Interval& xt = intervals[*x];
                    if(!xt.deleted) {
                        DLOG(INFO) << "remove collision (" << (*x) << ")";
                        xt.deleted = true;
                        assert(count > 0);
                        --count;
                    }
                }
            }
        }
    }
};

}}

#endif

