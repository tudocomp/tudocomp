#ifndef _INCLUDED_ESACOMP_LINEAR_HPP_
#define _INCLUDED_ESACOMP_LINEAR_HPP_

#include <vector>

#include <tudocomp/Env.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/Algorithm.hpp>

#include <tudocomp/ds/TextDS.hpp>

#include <sdsl/int_vector.hpp>

namespace tudocomp {
namespace lzss {

/// Implements a near-linear time and memory selection strategy for ESAComp.
class ESACompLinear: Algorithm {
    public:
        using Algorithm::Algorithm;

        inline static Meta meta() {
            Meta m("esacomp_strategy", "linear");
            return m;
        }

        void factorize(TextDS& t,
                       size_t fact_min,
                       std::vector<LZSSFactor>& out_factors) {

            auto n = t.size();

            auto& sa = t.require_sa();
            auto& isa = t.require_isa();
            auto& lcp = t.require_lcp();

            sdsl::bit_vector repl(n, 0);

            for(size_t i = 0; i < n; i++) {
                size_t len = lcp[i];
                if(len >= fact_min) {
                    size_t a = sa[i];
                    size_t b = sa[i-1];

                    //calculate potential and check if positions have been
                    //partially or fully replaced already
                    size_t pot_a = 0, pot_b = 0;
                    bool part_repl_a = false, part_repl_b = false;
                    bool full_repl_a = true, full_repl_b = true;

                    for(size_t j = 1; j < len; j++) {
                        size_t la = lcp[isa[a + j]];
                        if(la >= len) pot_a += 1;
                        part_repl_a = part_repl_a || repl[a + j];
                        full_repl_a = full_repl_a && repl[a + j];

                        size_t lb = lcp[isa[b + j]];
                        if(lb >= len) pot_b += 1;
                        part_repl_b = part_repl_b || repl[b + j];
                        full_repl_b = full_repl_b && repl[a + j];
                    }

                    //DLOG(INFO) << "i = " << (i+1);
                    //DLOG(INFO) << "a = " << (a+1) << ", pot_a = " << pot_a << ", partially: " << part_repl_a << ", fully: " << full_repl_a;
                    //DLOG(INFO) << "b = " << (b+1) << ", pot_b = " << pot_b << ", partially: " << part_repl_b << ", fully: " << full_repl_b;

                    size_t src, dst;
                    if(pot_a > pot_b) {
                        //b has less potential, prefer as destination
                        if(!part_repl_b && !full_repl_a) {
                            //b is still fully available, go ahead!
                            dst = b;
                            src = a; //TODO: what if a has been fully replaced?
                        } else if(!part_repl_a && !full_repl_b) {
                            //b has been partially replaced already, but a is available
                            dst = a;
                            src = b; //TODO: what if b has been fully replaced?
                        } else {
                            //no replacement possible
                            //DLOG(INFO) << "skip";
                            continue;
                        }
                    } else {
                        //a has less potential, prefer as destination
                        if(!part_repl_a && !full_repl_b) {
                            //a is still fully available, go ahead!
                            dst = a;
                            src = b; //TODO: what if a has been fully replaced?
                        } else if(!part_repl_b && !full_repl_a) {
                            //a has been partially replaced already, but b is available
                            dst = b;
                            src = a; //TODO: what if b has been fully replaced?
                        } else {
                            //no replacement possible
                            //DLOG(INFO) << "skip";
                            continue;
                        }
                    }

                    //introduce factor
                    //DLOG(INFO) << "factor: (" << (dst+1) << ", " << (src+1) << ", " << len << ")";
                    out_factors.push_back(LZSSFactor(dst, src, len));

                    //mark as replaced
                    for(size_t j = 0; j < len; j++) {
                        repl[dst + j] = 1;
                    }
                }
            }
        }
};

}}

#endif

