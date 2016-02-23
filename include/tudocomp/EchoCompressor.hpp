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

    /// Passes the input into the output.
    void compress(Input& in, Output& out) {
        if(in.has_size()) {
            m_coder.init(out, in.size());
        } else {
            m_coder.init(out);
        }
        
        auto in_guard = in.as_stream();
        std::istream& ins = *in_guard;
        
        char c;
        size_t pos = 0;
        while(ins.get(c)) {
            m_coder.encode(out, pos++, char32_t(c));
        }
        
        m_coder.finalize(out);
    }
    
    //TODO decompress

};

}

#endif
