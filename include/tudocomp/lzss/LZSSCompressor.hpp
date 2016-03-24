#ifndef _INCLUDED_LZSS_COMPRESSPR_HPP_
#define _INCLUDED_LZSS_COMPRESSPR_HPP_

#include <vector>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.h>
#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

template<typename C>
class LZSSCompressor : public Compressor {

protected:
    std::vector<LZSSFactor> m_factors;
    
private:
    C* m_coder;

public:
    inline LZSSCompressor() = delete;

    /// Construct the class with an environment.
    inline LZSSCompressor(Env& env) : Compressor(env) {
    }
    
    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void compress(Input& input, Output& output) override final {        
        //init factorization (possibly factorize offline to buffer)
        bool factorized = pre_factorize(input);
        
        //instantiate coder
        auto out_guard = output.as_stream();
        BitOStream out_bits(*out_guard);
        
        m_coder = new C(*m_env, input, out_bits, coder_opts(input));
        
        //pass factor buffer (empty or filled)
        m_coder->set_buffer(m_factors);
        
        //factorize
        if(!factorized) {
            factorize(input);
            handle_eof();
        }
        
        //encode
        if(factorized || m_coder->uses_buffer()) {
            
            auto in_guard = input.as_stream();
            std::istream& in_stream = *in_guard;
            
            //init
            m_coder->encode_init();
            
            //factors must be sorted by insert position!
            char c;
            size_t p = 0;
            
            for(auto f : m_factors) {
                while(p++ < f.pos) {
                    in_stream.get(c);
                    m_coder->encode_sym(uint8_t(c));
                }
                
                m_coder->encode_sym_flush();
                m_coder->encode_fact(f);
                
                size_t skip = f.num;
                while(skip--) in_stream.get(c);
                p += f.num;
            }
            
            size_t len = input.size();
            while(p++ < len) {
                in_stream.get(c);
                m_coder->encode_sym(uint8_t(c));
            }
            
            m_coder->encode_sym_flush();
        }
        
        //clean up
        delete m_coder;
        m_coder = nullptr;
    }

    /// Decompress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decompress(Input& input, Output& output) override {
        //TODO
    }
    
protected:
    void handle_fact(const LZSSFactor& f) {
        if(m_coder->uses_buffer()) {
            m_coder->buffer_fact(f);
        } else {
            m_coder->encode_sym_flush();
            m_coder->encode_fact(f);
        }
    }
    
    void handle_sym(uint8_t sym) {
        if(!m_coder->uses_buffer()) {
            m_coder->encode_sym(sym);
        }
    }
    
private:
    void handle_eof() {
        if(!m_coder->uses_buffer()) {
            m_coder->encode_sym_flush();
        }
    }

protected:
    virtual bool pre_factorize(Input& input) = 0;
    
    virtual LZSSCoderOpts coder_opts(Input& input) = 0;
    
    virtual void factorize(Input& input) = 0;
};

}}

#endif
