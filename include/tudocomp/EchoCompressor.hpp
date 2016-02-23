#ifndef _INCLUDED_ECHOCOMPRESSOR_HPP_
#define _INCLUDED_ECHOCOMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

namespace tudocomp {

/**
 * A simple compressor that echoes the input into the coder without
 * generating any factors whatsoever.
 */
template <typename C>
class EchoCompressor : public Compressor<C> {

public:
    EchoCompressor() = delete;
    
    /// Constructs the compressor from an environment.
    EchoCompressor(Env& env) : Compressor<C>(env) {
    }
    
    /// Destructs the compressor.
    ~EchoCompressor() {
    }

    /// Compresses the input.
    inline virtual void compress(Input& in, Output& out) override {
        C& coder = Compressor<C>::m_coder;
        
        if(in.has_size()) {
            coder.encode_init(out, in.size());
        } else {
            coder.encode_init(out);
        }
        
        auto in_guard = in.as_stream();
        std::istream& ins = *in_guard;
        
        char c;
        while(ins.get(c)) {
            coder.encode_sym(out, char32_t(c));
        }
        
        coder.encode_finalize(out);
    }

};

}

#endif
