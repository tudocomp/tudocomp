#pragma once

#include <tudocomp/compressors/lzss/LZSSOnlineCoding.hpp>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Computes the LZ77 factorization of the input by moving a sliding window
/// over it in which redundant phrases will be looked for.
template<typename coder_t>
class LZSSSlidingWindowCompressor : public Compressor {

private:
    size_t m_window;

public:
    inline static Meta meta() {
        Meta m("compressor", "lzss", "Lempel-Ziv-Storer-Szymanski (Sliding Window)");
        m.option("coder").templated<coder_t>("coder");
        m.option("window").dynamic(16);
        m.option("threshold").dynamic(3);
        return m;
    }

    /// Default constructor (not supported).
    inline LZSSSlidingWindowCompressor() = delete;

    /// Construct the class with an environment.
    inline LZSSSlidingWindowCompressor(Env&& e) : Compressor(std::move(e))
    {
        m_window = this->env().option("window").as_integer();
    }

    /// \copydoc Compressor::compress
    inline virtual void compress(Input& input, Output& output) override {
        auto ins = input.as_stream();

        typename coder_t::Encoder coder(env().env_for_option("coder"), output, NoLiterals());

        std::vector<uint8_t> buf;

        size_t ahead = 0; //marks the index in the buffer at which the back buffer ends and the ahead buffer begins
        char c;

        StatPhase phase("Factorize");

        //initially fill the buffer
        size_t buf_off = 0;
        while(buf.size() < 2 * m_window && ins.get(c)) {
            buf.push_back(uint8_t(c));
        }

        //factorize
        const len_t threshold = env().option("threshold").as_integer(); //factor threshold
        phase.log_stat("threshold", threshold);

        size_t pos = 0;
        bool eof = false;
        while(ahead < buf.size()) {
            size_t fpos = 0, fsrc = 0, fnum = 0;

            //walk back buffer
            for(size_t k = (ahead > m_window ? ahead - m_window : 0); k < ahead; k++) {
                //compare string
                size_t j = 0;
                while(ahead + j < buf.size() && buf[k + j] == buf[ahead + j]) {
                    ++j;
                }

                //factorize if longer than one already found
                if(j >= threshold && j > fnum) {
                    fpos = buf_off + ahead;
                    fsrc = buf_off + k;
                    fnum = j;
                }
            }

            //output longest factor or symbol
            size_t advance;

            if(fnum > 0) {
                // encode factor
                lzss::online_encode_factor(coder, fpos, fsrc, fnum, m_window);
                advance = fnum;
            } else {
                // encode literal
                lzss::online_encode_literal(coder, buf[ahead]);
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

    inline virtual void decompress(Input& input, Output& output) override {
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        auto text = lzss::online_decode(decoder, m_window);
        auto outs = output.as_stream();
        for(auto c : text) outs << c;
    }
};

} //ns

