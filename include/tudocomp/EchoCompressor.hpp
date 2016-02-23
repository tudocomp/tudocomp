#ifndef _INCLUDED_ECHOCOMPRESSOR_HPP_
#define _INCLUDED_ECHOCOMPRESSOR_HPP_

#include <tudocomp/compressor.h>

namespace tudocomp {

/**
 * A simple compressor that echoes the input into the coder without
 * generating any factors whatsoever.
 */
template <typename C>
class EchoCompressor {

private:
    C m_coder;

public:
    EchoCompressor() = delete;
    
    /// Constructs the compressor from an environment.
    EchoCompressor(Env& env) : m_coder(env) {
        //
    }
    
    /// Destructs the compressor.
    ~EchoCompressor() {
        //
    }

    /// Compresses the input.
    void compress(Input& in, Output& out) {
        if(in.has_size()) {
            m_coder.encode_init(out, in.size());
        } else {
            m_coder.encode_init(out);
        }
        
        auto in_guard = in.as_stream();
        std::istream& ins = *in_guard;
        
        char c;
        while(ins.get(c)) {
            m_coder.encode_sym(out, char32_t(c));
        }
        
        m_coder.encode_finalize(out);
    }
    
    /// Decompresses the input.
    void decompress(Input& in, Output& out) {
        m_coder.decode(in, out);
    }

};

}

#endif
