#ifndef _INCLUDED_COMPRESSOR_HPP
#define _INCLUDED_COMPRESSOR_HPP

#include <tudocomp/proto/Discard.hpp>

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

        //
        if(factor_coder.is_offline()) {
            DLOG(INFO) << "Factorize first, then encode...";
            Discard<uint8_t> discard;
            F::factorize(env, input, discard, factor_coder);
            //TODO encode
        } else {
            DLOG(INFO) << "Factorize and encode directly...";
            F::factorize(env, input, alphabet_coder, factor_coder);
        }
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
