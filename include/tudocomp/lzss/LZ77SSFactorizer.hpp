#ifndef _INCLUDED_LZ77SS_FACTORIZER_HPP
#define _INCLUDED_LZ77SS_FACTORIZER_HPP

#include <algorithm>
#include <vector>

#include <sdsl/int_vector.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

class LZ77SSFactorizer {

public:
    inline static std::vector<LZSSFactor>
    factorize(Env& env, const boost::string_ref& in, size_t len) {
        std::vector<LZSSFactor> factors;
        
        size_t fact_min = 3; //factor threshold
        size_t w = 16;       //window size
        
        LZSSFactor f;
        size_t p = 0;
        while(p < len) {
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
                factors.push_back(f);

                DLOG(INFO) << "Factor: {" << f.pos << "," << f.src << "," << f.num << "}";
                
                p += f.num;
                f = LZSSFactor(); //reset
            } else {
                ++p;
            }
        }
        
        //sorting is implicitly done already
        //std::sort(factors.begin(), factors.end());

        return factors;
    }

};

}}

#endif
