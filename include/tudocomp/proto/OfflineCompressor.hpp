#ifndef _INCLUDED_OFFLINE_COMPRESSOR_HPP
#define _INCLUDED_OFFLINE_COMPRESSOR_HPP

#include <sdsl/int_vector.hpp>
#include <sdsl/rrr_vector.hpp>

#include <tudocomp/Compressor.hpp>

namespace tudocomp {

// A requires: encode_alphabet(), encode_sym(uint8_t)
// C requires: encode_fact(F)
// F requires: PosComparator, pos, num
template<typename A, typename C, typename F>
class OfflineCompressor : public Compressor {

private:


public:
    /// Class needs to be constructed with an `Env&` argument.
    inline OfflineCompressor() = delete;

    /// Construct the class with an environment.
    inline OfflineCompressor(Env& env): Compressor(&env) {}
    
    /// Factorizes the input and marks the positions of generated factors
    virtual void factorize(const boost::string_ref& in, std::vector<F>& out_factors, sdsl::bit_vector& out_mark) = 0;
    
    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void compress(Input& input, Output& output) override final {
        
        //Read input into SDSL int vector buffer
        size_t len = input.size();

        auto in_guard = input.as_view();
        boost::string_ref& in_buf = *in_guard;
        
        //Factorize
        std::vector<F> factors;
        sdsl::bit_vector mark;
        
        factorize(in_buf, factors, mark);
        
        //Sort factors (TODO: optional?)
        {
            struct {
                bool operator()(F a, F b) {
                    return a.pos < b.pos;
                }
            } pos_comparator;
            
            std::sort(factors.begin(), factors.end(), pos_comparator);
        }
        
        //Encode
        auto out_guard = output.as_stream();
        std::ostream& out_stream = *out_guard;
        BitOStream out_bits(*out_guard);
        
        A alphabet_coder(*m_env, out_bits, in_buf);
        C factor_coder(*m_env, out_bits, factors);
        
        //TODO: write magic (ID for A and C)
        
        //Write input text length
        out_bits.write(len);
        
        //Write alphabet
        alphabet_coder.encode_alphabet();
        
        //Write rule markings
        {
            sdsl::rrr_vector<> mark_rrr(mark);
            mark_rrr.serialize(out_stream);
        }
        
        //Encode body
        size_t p = 0;
        for(F f : factors) {
            for(; p < f.pos; p++) {
                alphabet_coder.encode_sym(in_buf[p]);
            }
            
            factor_coder.encode_fact(f);
            p += f.num;
        }
        
        //Encode remainder
        while(p < len) {
            alphabet_coder.encode_sym(in_buf[p++]);
        }
        
        //Done
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
