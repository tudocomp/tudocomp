#ifndef _INCLUDED_OFFLINE_LZSS_CODER_HPP
#define _INCLUDED_OFFLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/lzss/LZSSUtil.hpp>

namespace tudocomp {
namespace lzss {

template<typename A>
class OfflineLZSSCoder {

private:
    BitOStream* m_out;
    Input* m_in;
    std::shared_ptr<A> m_alphabet_coder;

    std::vector<LZSSFactor> m_factors;
    
    bool m_src_use_delta;

    size_t m_num_min = SIZE_MAX;
    size_t m_num_max = 0;

    size_t m_src_max = 0;

    size_t m_num_bits = 0;
    size_t m_src_bits = 0;
    
    size_t m_encode_pos = 0;

public:
    static inline void require_offline() {
    }

    inline OfflineLZSSCoder(Env& env, Input& in, BitOStream& out, LZSSCoderOpts opts)
            : m_out(&out), m_in(&in), m_src_use_delta(opts.use_src_delta) {

        //TODO write magic
        out.write_compressed_int(in.size());
        out.writeBit(m_src_use_delta);
        
        m_alphabet_coder = std::shared_ptr<A>(new A(env, in, out));
    }
    
    inline ~OfflineLZSSCoder() {
        m_num_bits = bitsFor(m_num_max - m_num_min);
        m_src_bits = bitsFor(m_src_max);

        //Encode init
        m_out->write_compressed_int(m_num_min, 4);
        m_out->write_compressed_int(m_num_bits, 5);
        m_out->write_compressed_int(m_src_bits, 5);
        
        //Sort factors if list is unordered
        if(!m_src_use_delta) {
            std::sort(m_factors.begin(), m_factors.end());
        }

        //Encode
        encode_with_factors(*m_in, *this, *m_alphabet_coder, m_factors);
    }
    
    inline void encode_fact(const LZSSFactor& f) {
        m_factors.push_back(f);

        if(f.num < m_num_min) {
            m_num_min = f.num;
        }
        
        if(f.num > m_num_max) {
            m_num_max = f.num;
        }
        
        size_t src = m_src_use_delta ? (m_encode_pos - f.src) : f.src;
        if(src > m_src_max) {
            m_src_max = src;
        }
        
        m_encode_pos += f.num;
    }

    inline void encode_sym(uint8_t sym) {
        //don't encode symbols on the fly
        ++m_encode_pos;
    }

    inline void encode_fact_offline(const LZSSFactor& f) {
        m_out->writeBit(1);
        m_out->write(m_src_use_delta ? (m_encode_pos - f.src) : f.src, m_src_bits);
        m_out->write(f.num - m_num_min, m_num_bits);
    }
};

}}

#endif
