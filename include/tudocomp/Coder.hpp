#ifndef _INCLUDED_CODER_HPP_
#define _INCLUDED_CODER_HPP_

#include <tudocomp/env.h>
#include <tudocomp/io.h>

namespace tudocomp {

/// A coder for factors of type F.
template<typename F>
class Coder {

public:
    /// The default constructor is not supported.
    Coder() = delete;
    
    /// Constructor for an environment.
    Coder(Env& env) {
    }
    
    /// Destructor
    ~Coder() {
    }
    
    /// Initiates the encoding with no information about the original length.
    virtual void encode_init(Output& out) = 0;

    /// Initiates the encoding with information about the original length.
    virtual void encode_init(Output& out, size_t len) = 0;
    
    /// Encodes a raw symbol.
    virtual void encode_sym(Output& out, char32_t sym) = 0;
    
    /// Encodes a factor of the supported type.
    virtual void encode_fact(Output& out, const F& fact) = 0;
    
    /// Finalizes the encoding.
    virtual void encode_finalize(Output& out) = 0;
    
    /// Decodes and defactorizes the input
    virtual void decode(Input& in, Output& out) = 0;
};

}

#endif
