#ifndef _INCLUDED_LZSS_ESA_COMPRESSOR_HPP_
#define _INCLUDED_LZSS_ESA_COMPRESSOR_HPP_

#include <sdsl/int_vector.hpp>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/lcp.hpp>
#include <tudocomp/sdslex/int_vector_wrapper.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/MaxLCPSuffixList.hpp>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {


template<typename C>
class LZSSESACompressor : public Compressor {

public:
    inline LZSSESACompressor() = delete;

    /// Construct the class with an environment.
    inline LZSSESACompressor(Env& env) : Compressor(env) {
        C::require_offline();
    }
    
    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void compress(Input& input, Output& output) {
        auto in = input.as_view();

        size_t len = in.size();
        const uint8_t* in_ptr = in.mem_ptr();
        sdslex::int_vector_wrapper wrapper(in_ptr, len);

        //Construct SA
        sdsl::csa_bitcompressed<> sa;
        sdsl::construct_im(sa, wrapper.int_vector);

        //Construct ISA and LCP
        //TODO SDSL ???
        sdsl::int_vector<> isa(sa.size());
        sdsl::int_vector<> lcp(sa.size());

        for(size_t i = 0; i < sa.size(); i++) {
            isa[sa[i]] = i;

            if(i >= 2) {
                size_t j = sa[i], k = sa[i-1];
                while(in_ptr[j++] == in_ptr[k++]) ++lcp[i];
            }
        }

        sdsl::util::bit_compress(isa);
        sdsl::util::bit_compress(lcp);
        
        //Construct MaxLCPSuffixList
        size_t fact_min = 3; //factor threshold
        
        MaxLCPSuffixList<sdsl::csa_bitcompressed<>, sdsl::int_vector<>> list(sa, lcp, fact_min);

        //Instantiate coder
        auto out_guard = output.as_stream();
        BitOStream out_bits(*out_guard);

        C coder(*m_env, input, out_bits, LZSSCoderOpts(false, bitsFor(len)));

        //Factorize
        while(list.size() > 0) {
            //get suffix with longest LCP
            size_t m = list.first();
            
            //generate factor
            LZSSFactor fact(sa[m], sa[m-1], lcp[m]);
            coder.encode_fact(fact);
            DLOG(INFO) << "Factor: (" << fact.pos << ", " << fact.src << ", " << fact.num << ")";
            
            //remove overlapped entries
            for(size_t k = 0; k < fact.num; k++) {
                size_t i = isa[fact.pos + k];
                if(list.contains(i)) {
                    list.remove(i);
                }
            }
            
            //correct intersecting entries
            for(size_t k = 0; k < fact.num && fact.pos > k; k++) {
                size_t s = fact.pos - k - 1;
                size_t i = isa[s];
                if(list.contains(i)) {
                    if(s + lcp[i] > fact.pos) {
                        list.remove(i);
                        lcp[i] = fact.pos - s;
                        list.insert(i);
                    }
                }
            }
        }
    }
    
    /// Decompress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decompress(Input& input, Output& output) {
        //TODO
    }
    
};

}}

#endif
