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

    size_t m_len;    
    std::vector<LZSSFactor> m_factors;

    size_t m_num_min = SIZE_MAX;
    size_t m_num_max = 0;

    size_t m_src_max = 0;

    size_t m_num_bits = 0;
    size_t m_src_bits = 0;

public:
    inline OfflineLZSSCoder(Env& env, BitOStream& out, size_t input_len) : m_out(&out), m_len(input_len) {
    }
    
    inline ~OfflineLZSSCoder() {
    }
    
    inline bool is_offline() {
        return true;
    }

    inline void operator()(const LZSSFactor& f) {
        m_factors.push_back(f);

        if(f.num < m_num_min) {
            m_num_min = f.num;
        }
        
        if(f.num > m_num_max) {
            m_num_max = f.num;
        }
        
        if(f.src > m_src_max) {
            m_src_max = f.src;
        }
    }

    template<typename A>
    inline void encode_offline(Input& input, A& encode_sym) {
        m_num_bits = bitsFor(m_num_max - m_num_min);
        m_src_bits = bitsFor(m_src_max);

        //Encode init
        m_out->write_compressed_int(m_num_min, 4);
        m_out->write_compressed_int(m_num_bits, 5);
        m_out->write_compressed_int(m_src_bits, 5);

        auto in_guard = input.as_stream();
        std::istream& ins = *in_guard;

        //Encode factors
        size_t p = 0;
        char c;
        for(LZSSFactor f : m_factors) {
            while(p < f.pos) {
                if(ins.get(c)) {
                    encode_sym(uint8_t(c));
                }
                ++p;
            }

            encode_fact(f);
            p += f.num;

            //skip
            size_t num = f.num;
            while(num--) {
                ins.get(c);
            }
        }

        //Encode remainder
        DLOG(INFO) << "remainder (p = " << p << ", len = " << m_len << ")";
        while(p < m_len) {
            if(ins.get(c)) {
                encode_sym(uint8_t(c));
            }
            ++p;
        }
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
