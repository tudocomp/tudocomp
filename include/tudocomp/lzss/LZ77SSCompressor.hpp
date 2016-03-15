#ifndef _INCLUDED_LZ77SS_COMPRESSOR_HPP
#define _INCLUDED_LZ77SS_COMPRESSOR_HPP

#include <algorithm>
#include <functional>
#include <vector>

#include <boost/circular_buffer.hpp>

#include <sdsl/int_vector.hpp>

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

template<typename C>
class LZ77SSCompressor : public Compressor {

public:
    inline LZ77SSCompressor() = delete;

    /// Construct the class with an environment.
    inline LZ77SSCompressor(Env& env) : Compressor(env) {
    }
    
    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void compress(Input& input, Output& output) {

        auto out_guard = output.as_stream();
        BitOStream out_bits(*out_guard);

        size_t fact_min = 3; //factor threshold
        size_t w = 16;       //window size
        
        C coder(*m_env, input, out_bits);
        coder.use_src_delta(true);
        coder.use_src_bits(bitsFor(w)); //source can refer to beginning of window at max

        auto in_guard = input.as_stream();
        std::istream& ins = *in_guard;

        boost::circular_buffer<uint8_t> buf(2 * w);

        size_t ahead = 0; //marks the index in the buffer at which the back buffer ends and the ahead buffer begins

        char c;

        //initially fill the buffer
        size_t buf_off = 0;
        while(buf.size() < 2 * w && ins.get(c)) {
            buf.push_back(uint8_t(c));
        }

        bool eof = false;
        while(!eof || ahead < buf.size()) {
            LZSSFactor f;

            //walk back buffer
            for(size_t k = (ahead > w ? ahead - w : 0); k < ahead; k++) {
                //compare string
                size_t j = 0;
                while(ahead + j < buf.size() && buf[k + j] == buf[ahead + j]) {
                    ++j;
                }

                //factorize if longer than one already found
                if(j >= fact_min && j > f.num) {
                    f.pos = buf_off + ahead;
                    f.src = buf_off + k;
                    f.num = j;
                }
            }

            //output longest factor or symbol
            size_t advance;

            if(f.num > 0) {
                DLOG(INFO) << "Factor: {" << f.pos << "," << f.src << "," << f.num << "}";
                coder.encode_fact(f);
                advance = f.num;
            } else {
                coder.encode_sym(buf[ahead]);
                advance = 1;
            }

            //advance buffer
            while(advance--) {
                if(ahead < w) {
                    //case 1: still reading the first w symbols from the stream
                    ++ahead;
                } else if(!eof && ins.get(c)) {
                    //case 2: read a new symbol
                    buf.pop_front();
                    buf.push_back(uint8_t(c)); //TODO is this how to use circular buffers? ...
                    ++buf_off;
                } else {
                    //case 3: EOF, read rest of buffer
                    eof = true;
                    ++ahead;
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
