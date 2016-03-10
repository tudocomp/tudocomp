#ifndef _INCLUDED_OFFLINE_LZSS_CODER_HPP
#define _INCLUDED_OFFLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

template<typename A>
class OfflineLZSSCoder {

private:
    BitOStream* m_out;
    Input* m_in;
    std::shared_ptr<A> m_alphabet_coder;

    std::vector<LZSSFactor> m_factors;

    size_t m_num_min = SIZE_MAX;
    size_t m_num_max = 0;

    size_t m_src_max = 0;

    size_t m_num_bits = 0;
    size_t m_src_bits = 0;

public:
    inline OfflineLZSSCoder(Env& env, Input& in, BitOStream& out)
            : m_out(&out), m_in(&in) {

        //TODO write magic
        out.write_compressed_int(in.size());
        m_alphabet_coder = std::shared_ptr<A>(new A(env, in, out));
    }
    
    inline ~OfflineLZSSCoder() {
        size_t len = m_in->size();

        m_num_bits = bitsFor(m_num_max - m_num_min);
        m_src_bits = bitsFor(m_src_max);

        //Encode init
        m_out->write_compressed_int(m_num_min, 4);
        m_out->write_compressed_int(m_num_bits, 5);
        m_out->write_compressed_int(m_src_bits, 5);

        auto in_guard = m_in->as_stream();
        std::istream& ins = *in_guard;

        //Encode factors
        size_t p = 0;
        char c;
        for(LZSSFactor f : m_factors) {
            while(p < f.pos) {
                if(ins.get(c)) {
                    (*m_alphabet_coder)(uint8_t(c));
                }
                ++p;
            }

            encode_fact_offline(f);
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
                (*m_alphabet_coder)(uint8_t(c));
            }
            ++p;
        }
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

    inline void operator()(uint8_t sym) {
        //don't encode symbols on the fly
    }

    inline void encode_fact_offline(const LZSSFactor& f) {
        DLOG(INFO) << "encode_fact_offline({" << f.pos << "," << f.src << "," << f.num << "})";
        m_out->writeBit(1);
        m_out->write(f.src, m_src_bits);
        m_out->write(f.num - m_num_min, m_num_bits);
    }
};

}}

#endif
