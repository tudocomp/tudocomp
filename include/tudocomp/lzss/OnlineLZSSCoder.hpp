#ifndef _INCLUDED_ONLINE_LZSS_CODER_HPP
#define _INCLUDED_ONLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

class OnlineLZSSCoder {

private:
    BitOStream* m_out;
    
    size_t m_src_bits;
    size_t m_num_bits;

public:
    inline OnlineLZSSCoder(Env& env, BitOStream& out, size_t input_len) : m_out(&out) {
        m_src_bits = bitsFor(input_len);
        m_num_bits = bitsFor(input_len);
    }
    
    inline void encode_fact(const LZSSFactor& f) {
        DLOG(INFO) << "encode_fact({" << f.pos << "," << f.src << "," << f.num << "})";
        m_out->writeBit(1);
        m_out->write(f.src, m_src_bits);
        m_out->write(f.num, m_num_bits);
    }
};

}}

#endif
