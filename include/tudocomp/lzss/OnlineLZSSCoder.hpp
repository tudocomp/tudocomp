#ifndef _INCLUDED_ONLINE_LZSS_CODER_HPP
#define _INCLUDED_ONLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

template<typename A>
class OnlineLZSSCoder {

private:
    BitOStream* m_out;
    std::shared_ptr<A> m_alphabet_coder;
    
    bool m_src_use_delta = false;
    size_t m_encode_pos = 0;
    
    size_t m_src_bits;
    size_t m_num_bits;

public:
    inline OnlineLZSSCoder(Env& env, Input& in, BitOStream& out)
            : m_out(&out) {

        size_t len = in.size();
        m_src_bits = bitsFor(len);
        m_num_bits = bitsFor(len);

        //TODO write magic
        out.write_compressed_int(len);
        m_alphabet_coder = std::shared_ptr<A>(new A(env, in, out));
    }

    inline ~OnlineLZSSCoder() {
    }
    
    inline void use_src_delta(bool b) {
        m_src_use_delta = b;
    }
    
    inline void use_src_bits(size_t bits) {
        m_src_bits = bits;
    }
    
    inline void encode_fact(const LZSSFactor& f) {
        m_alphabet_coder->encode_sym_flush();
        
        DLOG(INFO) << "encode_fact({" << f.pos << "," << f.src << "," << f.num << "})";
        m_out->writeBit(1);
        m_out->write(m_src_use_delta ? (m_encode_pos - f.src) : f.src, m_src_bits);
        m_out->write(f.num, m_num_bits);
        m_encode_pos += f.num;
    }

    inline void encode_sym(uint8_t sym) {
        m_alphabet_coder->encode_sym(sym);
        ++m_encode_pos;
    }
};

}}

#endif
