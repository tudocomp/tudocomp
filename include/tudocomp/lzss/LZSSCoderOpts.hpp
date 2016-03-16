#ifndef _INCLUDED_LZSS_CODER_OPTS_HPP
#define _INCLUDED_LZSS_CODER_OPTS_HPP

namespace tudocomp {
namespace lzss {
    
struct LZSSCoderOpts {

    bool use_src_delta;
    size_t src_bits;
    
    LZSSCoderOpts(bool _use_src_delta)
        : use_src_delta(_use_src_delta), src_bits(64) {
    }
    
    LZSSCoderOpts(bool _use_src_delta, size_t _src_bits)
        : use_src_delta(_use_src_delta), src_bits(_src_bits) {
    }

};
    
}}

#endif
