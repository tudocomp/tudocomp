/*
    This is a simple template for new coders and is not meant to be included
    or inherited.
    
    It serves as an overview over which functions MUST be supported by coders.
*/
#undef _INCLUDED_CODER_HPP_
#define _INCLUDED_CODER_HPP_

#include <tudocomp/env.h>
#include <tudocomp/io.h>

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
    void encode_init(Output& out);

    /// Initiates the encoding with information about the original length.
    void encode_init(Output& out, size_t len);
    
    /// Encodes a raw symbol.
    void encode_sym(Output& out, char32_t sym);
    
    /// Encodes a factor of the supported type.
    void encode_fact(Output& out, const F& fact);
    
    /// Finalizes the encoding.
    void encode_finalize(Output& out);
    
    /// Decodes and defactorizes the input
    void decode(Input& in, Output& out);
};

#endif
