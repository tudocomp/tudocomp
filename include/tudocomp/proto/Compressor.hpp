#ifndef _INCLUDED_COMPRESSOR_HPP
#define _INCLUDED_COMPRESSOR_HPP

#include <tudocomp/Compressor.hpp>

namespace tudocomp {

template<typename F, typename A, typename C>
inline static void factor_compress(Env& env, Input& input, Output& output) {
    //Read input into SDSL int vector buffer
    size_t len = input.size(); //TODO check has_size first
    
    DLOG(INFO) << "Init (n = " << len << ")...";
    
    //Encode
    DLOG(INFO) << "Init encoding...";
    
    auto out_guard = output.as_stream();
    BitOStream out_bits(*out_guard);
    
    //TODO: write magic (ID for A and C)

    //Write input text length
    out_bits.write_compressed_int(len);
    
    {
        //Init coders
        A alphabet_coder(env, input, out_bits);
        C factor_coder(env, out_bits, len);
        
        //Factorize and encode
        DLOG(INFO) << "Factorize / encode...";
        F::factorize(env, input, alphabet_coder, factor_coder);
    }
    
    //Done
    out_bits.flush();
    DLOG(INFO) << "Done.";
}

inline static void factor_decompress(Env& env, Input& input, Output& output) {
    //TODO
}

}

#endif
