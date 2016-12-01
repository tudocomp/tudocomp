#ifndef _INCLUDED_RUN_LENGTH_ENCODER_HPP
#define _INCLUDED_RUN_LENGTH_ENCODER_HPP

#include <tudocomp/def.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>

namespace tdc {

template<typename coder_t>
class RunLengthEncoder : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "rle", "Run-length encoding");
        m.option("coder").templated<coder_t>();
        m.option("min_run").dynamic("3");
        return m;
    }

    inline RunLengthEncoder(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
        // setup I/O
        auto ins = input.as_stream();

        // instantiate coder
        typename coder_t::Encoder coder(env().env_for_option("coder"), output, NoLiterals()); //TODO: literal iterator for input streams

        // define working variables
        len_t min_run = env().option("min_run").as_integer();
        uliteral_t run_char = 0;
        len_t run_length = 0;

        // writes the current run to the output stream
        auto emit_run = [&]() {
            if (run_length >= min_run) {
                // encode the character twice
                coder.encode(run_char, literal_r);
                coder.encode(run_char, literal_r);

                // encode run length
                coder.encode(true, bit_r); // yes, this is an encoded run
                coder.encode(run_length - 2U, len_r);

                // reset
                run_length = 0;
            } else if(run_length > 0) {
                // write the whole run
                // each two characters, encode a zero bit to indicate that this
                // is no encoded run
                bool flip = false;
                while(run_length--) {
                    coder.encode(run_char, literal_r);

                    if(flip) {
                        coder.encode(false, bit_r);
                    }

                    flip = !flip;
                }
            }
        };

        // process input
        for(auto c : ins) {
            auto x = uliteral_t(c); // cast to unsigned!
            if(run_length > 0 && x == run_char) {
                run_length++;
            } else {
                emit_run();
                run_char = x;
                run_length = 1;
            }
        }

        // emit final run
        emit_run();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        // setup I/O
        auto outs = output.as_stream();

        // instantiate decoder
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);

        // decode
        bool flip = false;
        uliteral_t run_char = 0;

        while(!decoder.eof()) {
            auto c = decoder.template decode<uliteral_t>(literal_r);
            outs << c;

            if(flip && c == run_char) {
                flip = false;
                bool is_run = decoder.template decode<bool>(bit_r);
                if(is_run) {
                    len_t run_length = decoder.template decode<len_t>(len_r);
                    while(run_length--) outs << c; // repeat
                }
            } else {
                flip = true;
                run_char = c;
            }
        }
    }
};

}

#endif
