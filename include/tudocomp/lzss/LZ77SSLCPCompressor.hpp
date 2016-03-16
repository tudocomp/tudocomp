#ifndef _INCLUDED_LZ77SS_LCP_COMPRESSOR_HPP
#define _INCLUDED_LZ77SS_LCP_COMPRESSOR_HPP

#include <algorithm>
#include <functional>
#include <vector>

#include <sdsl/int_vector.hpp>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/lcp.hpp>
#include <tudocomp/sdslex/int_vector_wrapper.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

template<typename C>
class LZ77SSLCPCompressor : public Compressor {

public:
    inline LZ77SSLCPCompressor() = delete;

    /// Construct the class with an environment.
    inline LZ77SSLCPCompressor(Env& env) : Compressor(env) {
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
        
        //Debug output
        /*
        DLOG(INFO) << "SA (" << sa.size() << " entries):";
        for(size_t i = 0; i < sa.size(); i++) DLOG(INFO) << "sa[" << i << "] = " << sa[i];

        DLOG(INFO) << "LCP (" << lcp.size() << " entries):";
        for(size_t i = 0; i < lcp.size(); i++) DLOG(INFO) << "lcp[" << i << "] = " << lcp[i];
        */

        //Instantiate coder
        size_t fact_min = 3; //factor threshold
        
        auto out_guard = output.as_stream();
        BitOStream out_bits(*out_guard);

        C coder(*m_env, input, out_bits, LZSSCoderOpts(true, bitsFor(len)));

        //Factorize
        for(size_t i = 0; i < len;) {
            //get SA position for suffix i
            size_t h = isa[i];

            //search "upwards" in LCP array
            //include current, exclude last
            size_t p1 = lcp[h];
            ssize_t h1 = h - 1;
            if (p1 > 0) {
                while (h1 >= 0 && sa[h1] > sa[h]) {
                    p1 = std::min(p1, size_t(lcp[h1--]));
                }
            }

            //search "downwards" in LCP array
            //exclude current, include last
            size_t p2 = 0;
            size_t h2 = h + 1;
            if (h2 < len) {
                p2 = SSIZE_MAX;
                do {
                    p2 = std::min(p2, size_t(lcp[h2]));
                    if (sa[h2] < sa[h]) {
                        break;
                    }
                } while (++h2 < len);

                if (h2 >= len) {
                    p2 = 0;
                }
            }

            //select maximum
            size_t p = std::max(p1, p2);
            if (p >= fact_min) {
                coder.encode_fact(LZSSFactor(i, sa[p == p1 ? h1 : h2], p));
                i += p; //advance
            } else {
                coder.encode_sym(in_ptr[i]);
                ++i; //advance
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
