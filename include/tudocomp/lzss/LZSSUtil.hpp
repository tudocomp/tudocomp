#ifndef _INCLUDED_LZSS_UTIL_HPP
#define _INCLUDED_LZSS_UTIL_HPP

#include <tudocomp/io.h>

namespace tudocomp {
namespace lzss {

    template<typename C, typename A, typename F>
    void encode_offline(Input& in, C& coder, A& alphabet_coder, const F& sorted_factors) {
        //TODO byte alphabets only
        size_t len = in.size();
        
        auto in_guard = in.as_stream();
        std::istream& ins = *in_guard;

        //Encode factors
        size_t p = 0;
        char c;
        for(auto f : sorted_factors) {
            while(p < f.pos) {
                if(ins.get(c)) {
                    alphabet_coder.encode_sym(uint8_t(c));
                }
                ++p;
            }

            coder.encode_fact_offline(f);
            p += f.num;

            //skip
            size_t num = f.num;
            while(num--) {
                ins.get(c);
            }
        }

        //Encode remainder
        while(p < len) {
            if(ins.get(c)) {
                alphabet_coder.encode_sym(uint8_t(c));
            }
            ++p;
        }
    }
   
}
}

#endif