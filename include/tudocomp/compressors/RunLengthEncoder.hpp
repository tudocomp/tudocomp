#ifndef _INCLUDED_RUN_LENGTH_ENCODER_HPP
#define _INCLUDED_RUN_LENGTH_ENCODER_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>

namespace tdc {

template<typename coder_t, typename len_t = uint32_t>
class RunLengthEncoder : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "rle", "Run-length encoding");
        m.option("min_run").dynamic("3");
        m.option("coder").templated<coder_t>();
        return m;
    }

    inline RunLengthEncoder(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
        // setup I/O
        auto ins = input.as_stream();
        auto outs = output.as_stream();
        BitOStream out_bits(outs);

        // instantiate coder
        coder_t coder(env().env_for_option("coder"), out_bits);

        // define ranges
        TypeRange<len_t> r_len;
        CharRange r_char;
        BitRange r_bits;

        // define working variables
        len_t min_run = env().option("min_run").as_integer();
        uint8_t run_char;
        len_t run_length = 0;

        // writes the current run to the output stream
        auto emit_run = [&]() {
            if (run_length >= min_run) {
                // encode the character twice
                coder.encode(run_char, r_char);
                coder.encode(run_char, r_char);

                // encode run length
                coder.encode(true, r_bits); // yes, this is an encoded run
                coder.encode(run_length - 2U, r_len);

                // reset
                run_length = 0;
            } else if(run_length > 0) {
                // write the whole run
                // each two characters, encode a zero bit to indicate that this
                // is no encoded run
                bool flip = false;
                while(run_length--) {
                    coder.encode(run_char, r_char);

                    if(flip) {
                        coder.encode(run_char, r_char);
                        coder.encode(false, r_bits);
                    }

                    flip = !flip;
                }
            }
        };

        // process input
        for(uint8_t c : ins) {
            if(run_length > 0 && c == run_char) {
                run_length++;
            } else {
                emit_run();
                run_char = c;
                run_length = 1;
            }
        }

        // emit final run
        emit_run();
    }

    inline virtual void decompress(Input& input, Output& output) override {
    }
};

}

#endif
