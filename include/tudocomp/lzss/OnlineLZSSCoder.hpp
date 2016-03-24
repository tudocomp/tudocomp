#ifndef _INCLUDED_ONLINE_LZSS_CODER_HPP
#define _INCLUDED_ONLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

template<typename A>
class OnlineLZSSCoder {

private:
    BitOStream* m_out;
    std::shared_ptr<A> m_alphabet_coder;
    
    bool m_src_use_delta;
    size_t m_src_bits;
    size_t m_num_bits;
    
public:
    inline OnlineLZSSCoder(Env& env, Input& in, BitOStream& out, LZSSCoderOpts opts)
            : m_out(&out) {

        size_t len = in.size();
        m_src_bits = std::min(bitsFor(len), opts.src_bits);
        m_num_bits = bitsFor(len);
        m_src_use_delta = opts.use_src_delta;

        //TODO write magic
        out.write_compressed_int(len);
        out.writeBit(m_src_use_delta);
        
        m_alphabet_coder = std::shared_ptr<A>(new A(env, in, out));
    }
    
    inline void encode_init() {
    }
    
    inline void encode_fact(const LZSSFactor& f) {
        m_alphabet_coder->encode_sym_flush();
        
        m_out->writeBit(1);
        m_out->write(m_src_use_delta ? (f.pos - f.src) : f.src, m_src_bits);
        m_out->write(f.num, m_num_bits);
    }

    inline void encode_sym(uint8_t sym) {
        m_alphabet_coder->encode_sym(sym);
    }
    
    inline void encode_sym_flush() {
        m_alphabet_coder->encode_sym_flush();
    }
    
    inline bool uses_buffer() {
        return false;
    }
    
    template<typename T>
    inline void set_buffer(T& buffer) {
    }
    
    inline void buffer_fact(const LZSSFactor& f) {
    }
};

}}

#endif
