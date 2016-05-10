/*
    This is a sample implementation for an online coder. In the future, this
    will be replaced by a reference implementation of the original LZSS
    algorithm.
*/

#ifndef _INCLUDED_LZ77SS_SLIDING_WINDOW_COMPRESSOR_HPP
#define _INCLUDED_LZ77SS_SLIDING_WINDOW_COMPRESSOR_HPP

#include <algorithm>
#include <vector>

#include <tudocomp/util.h>
#include <tudocomp/lzss/LZSSCompressor.hpp>

namespace tudocomp {
namespace lzss {

const std::string WINDOW_OPTION = "lzss.window";

/// Computes the LZ77 factorization of the input by moving a sliding window
/// over it in which redundant phrases will be looked for.
template<typename C>
class LZ77SSSlidingWindowCompressor : public LZSSCompressor<C> {

private:
    size_t m_window;

public:
    /// Default constructor (not supported).
    inline LZ77SSSlidingWindowCompressor() = delete;

    /// Construct the class with an environment.
    inline LZ77SSSlidingWindowCompressor(Env& env) : LZSSCompressor<C>(env) {
        m_window = env.option_as<size_t>(WINDOW_OPTION, 16);
    }

protected:
    /// \copydoc
    inline virtual bool pre_factorize(Input& input) override {
        return false;
    }

    /// \copydoc
    inline virtual LZSSCoderOpts coder_opts(Input& input) override {
        return LZSSCoderOpts(true, bitsFor(m_window));
    }

    /// \copydoc
    inline virtual void factorize(Input& input) override {
        auto in_guard = input.as_stream();
        std::istream& ins = *in_guard;

        std::vector<uint8_t> buf;

        size_t ahead = 0; //marks the index in the buffer at which the back buffer ends and the ahead buffer begins
        char c;

        //initially fill the buffer
        size_t buf_off = 0;
        while(buf.size() < 2 * m_window && ins.get(c)) {
            buf.push_back(uint8_t(c));
        }

        //factorize
        size_t fact_min = 3; //factor threshold

        size_t pos = 0;
        bool eof = false;
        while(ahead < buf.size()) {
            LZSSFactor f;

            //walk back buffer
            for(size_t k = (ahead > m_window ? ahead - m_window : 0); k < ahead; k++) {
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
                LZSSCompressor<C>::handle_fact(f);
                advance = f.num;
            } else {
                LZSSCompressor<C>::handle_sym(buf[ahead]);
                advance = 1;
            }

            //advance buffer
            pos += advance;
            while(advance--) {
                if(ahead < m_window) {
                    //case 1: still reading the first w symbols from the stream
                    ++ahead;
                } else if(!eof && ins.get(c)) {
                    //case 2: read a new symbol
                    buf.erase(buf.begin()); //TODO ouch
                    buf.push_back(uint8_t(c));
                    ++buf_off;
                } else {
                    //case 3: EOF, read rest of buffer
                    eof = true;
                    ++ahead;
                }
            }
        }
    }
};

}}

#endif
