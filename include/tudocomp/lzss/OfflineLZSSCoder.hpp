#ifndef _INCLUDED_OFFLINE_LZSS_CODER_HPP
#define _INCLUDED_OFFLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

class OfflineLZSSCoder {

private:
    BitOStream* m_out;
    size_t m_num_min;
    
    size_t m_num_bits;
    size_t m_src_bits;

public:
    inline OfflineLZSSCoder(Env& env, BitOStream& out, const std::vector<LZSSFactor>& factors) : m_out(&out) {
        m_num_min = SIZE_MAX;
        
        size_t num_max = 0;
        size_t src_max = 0;
        
        for(LZSSFactor f : factors) {
            if(f.num < m_num_min) {
                m_num_min = f.num;
            }
            
            if(f.num > num_max) {
                num_max = f.num;
            }
            
            if(f.src > src_max) {
                src_max = f.src;
            }
        }
        
        m_num_bits = bitsFor(num_max - m_num_min);
        m_src_bits = bitsFor(src_max);
    }
    
    inline void encode_init() {
        m_out->write_compressed_int(m_num_min, 4);
        m_out->write_compressed_int(m_num_bits, 5);
        m_out->write_compressed_int(m_src_bits, 5);
    }
    
    inline void encode_fact(const LZSSFactor& f) {
        DLOG(INFO) << "encode_fact({" << f.pos << "," << f.src << "," << f.num << "})";
        m_out->writeBit(1);
        m_out->write(f.src, m_src_bits);
        m_out->write(f.num - m_num_min, m_num_bits);
    }
};

}}

#endif
