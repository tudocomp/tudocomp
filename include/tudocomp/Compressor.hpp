#ifndef _INCLUDED_COMPRESSOR_HPP_
#define _INCLUDED_COMPRESSOR_HPP_

#include <tudocomp/env.h>
#include <tudocomp/io.h>

namespace tudocomp {

/// A compressor, using coder type C.
template<typename C>
class Compressor {

protected:
    C m_coder;

public:
    /// The default constructor is not supported.
    Compressor() = delete;
    
    /// Constructor for an environment.
    Compressor(Env& env) : m_coder(env) {
    }
    
    /// Destructor
    ~Compressor() {
    }
    
    /// Compresses the input.
    virtual void compress(Input& in, Output& out) = 0;
    
    /// Decompresses the input.
    inline virtual void decompress(Input& in, Output& out) {
        m_coder.decode(in, out);
    }
};

}

#endif
