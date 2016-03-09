#ifndef _INCLUDED_OFFLINE_COMPRESSOR_HPP
#define _INCLUDED_OFFLINE_COMPRESSOR_HPP

#include <sdsl/int_vector.hpp>
#include <sdsl/rrr_vector.hpp>

#include <tudocomp/Compressor.hpp>

namespace tudocomp {

// A requires: encode_alphabet(), encode_sym(uint8_t)
// C requires: encode_init, encode_fact(factor_type)
// F requires: static factorize_offline
// factor_type requires: pos
template<typename F, typename A, typename C>
class OfflineCompressor : public Compressor {

private:

public:
    /// Class needs to be constructed with an `Env&` argument.
    inline OfflineCompressor() = delete;

    /// Construct the class with an environment.
    inline OfflineCompressor(Env& env): Compressor(env) {}
    
    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void compress(Input& input, Output& output) override final {
        
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
            //Factorize
            DLOG(INFO) << "Factorize...";
            auto factors = F::factorize_offline(*m_env, input);

            //Init coders
            A alphabet_coder(*m_env, out_bits, input);
            C factor_coder(*m_env, out_bits, factors);

            //Encode body
            auto in_view = input.as_view();

            size_t p = 0;
            for(auto f : factors) {
                while(p < f.pos) {
                    alphabet_coder.encode_sym((*in_view)[p++]);
                }
                
                factor_coder.encode_fact(f);
                p = f.pos + f.num;
            }
            
            //Encode remainder
            while(p < len) {
                alphabet_coder.encode_sym((*in_view)[p++]);
            }
        }
        
        //Done
        out_bits.flush();
        DLOG(INFO) << "Done.";
    }

    /// Decompress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decompress(Input& input, Output& output) override final {
        // TODO
    }
};

}

#endif
