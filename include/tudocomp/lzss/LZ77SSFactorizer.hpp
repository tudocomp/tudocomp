#ifndef _INCLUDED_LZ77SS_FACTORIZER_HPP
#define _INCLUDED_LZ77SS_FACTORIZER_HPP

#include <algorithm>
#include <functional>
#include <vector>

#include <sdsl/int_vector.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/proto/Collector.hpp>
#include <tudocomp/proto/Discard.hpp>

#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

class LZ77SSFactorizer {

public:
    template<typename A, typename C>
    inline static void factorize(
        Env& env, const boost::string_ref& in, size_t len, A& consume_sym, C& consume_fact) {
        
        size_t fact_min = 3; //factor threshold
        size_t w = 16;       //window size

        size_t p = 0;
        while(p < len) {
            LZSSFactor f;
            
            for(size_t k = (p > w ? p - w : 0); k + fact_min < p; k++) {
                size_t j = 0;
                while(p + j < len && in[k + j] == in[p + j]) {
                    ++j;
                }
                
                if(j >= fact_min && j > f.num) {
                    f.pos = p;
                    f.src = k;
                    f.num = j;
                }
            }
            
            if(f.num > 0) {
                DLOG(INFO) << "Factor: {" << f.pos << "," << f.src << "," << f.num << "}";
                
                consume_fact(f);
                p += f.num;
            } else {
                consume_sym(in[p]);
                ++p;
            }
        }
    }

    inline static std::vector<LZSSFactor>
    factorize_offline(Env& env, const boost::string_ref& in, size_t len) {
        Discard<uint8_t> syms;
        Collector<LZSSFactor> factors;
        
        factorize(env, in, len, syms, factors);
        return factors.vector;
    }

};

}}

#endif
