#ifndef _INCLUDED_EXAMPLE_COMPRESSOR_HPP_
#define _INCLUDED_EXAMPLE_COMPRESSOR_HPP_

#include <tudocomp/tudocomp.hpp>

namespace tdc {

class ExampleCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "example_compressor",
               "This is an example compressor.");

        //Define options
        m.option("minimum_run").dynamic("3");
        m.option("rle_symbol").dynamic("%");

        return m;
    }

    inline ExampleCompressor(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
        auto istream = input.as_stream(); // retrieve the input stream
        auto ostream = output.as_stream(); // retrieve the output stream

        char current; // stores the current character read from the input
        char last; //stores the character that preceded the current character
        size_t counter = 0; // counts the length of the run of the current character

        // read the option values
        auto minimum_run = env().option("minimum_run").as_integer();
        auto rle_symbol = env().option("rle_symbol").as_string();

        // writes the current run to the output stream
        auto emit_run = [&]() {
            if (counter >= minimum_run) {
                // if the run exceeds the minimum amount of characters,
                // encode the run using using the RLE symbol syntax
                ostream << last << rle_symbol << counter << rle_symbol;
            } else {
                // otherwise, do not encode and emit the whole run
                for (size_t i = 0; i <= counter; i++) {
                    ostream << last;
                }
            }
        };

        // retrieve the first character on the stream
        if (istream.get(last)) {
            // continue reading from the stream
            while(istream.get(current)) {
                if (current == last) {
                    // increase length of the current run
                    counter++;
                } else {
                    // emit the previous run
                    emit_run();

                    // continue reading from the stream, starting a new run
                    last = current;
                    counter = 0;
                }
            }

            // emit the final run
            emit_run();
        }
    }

    inline virtual void decompress(Input& input, Output& output) override {
        // read the option values
        auto rle_symbol = env().option("rle_symbol").as_string()[0];

        auto iview = input.as_view(); // retrieve the input as a view (merely for educational reasons)
        auto ostream = output.as_stream(); // retrieve the output stream

        char last = '?'; // stores the last character read before a "%n%" pattern is encountered

        // process the input
        for (size_t i = 0; i < iview.size(); i++) {
            if (iview[i] == rle_symbol) {
                // after encountering the '%' chracter, parse the following characters
                // as a decimal number until the next '%' is encountered
                size_t counter = 0;
                for (i++; i < iview.size(); i++) {
                    if (iview[i] == rle_symbol) {
                        // conclusion of the "%n%" pattern
                        break;
                    } else {
                        // naive decimal number parser
                        counter *= 10;
                        counter += (iview[i] - '0');
                    }
                }

                // repeat the previous character according to the parsed length
                for (size_t x = 0; x < counter; x++) {
                    ostream << last;
                }
            } else {
                // output any character not part of a "c%n%" pattern and continue reading
                ostream << iview[i];
                last = iview[i];
            }
        }
    }

};

}

#endif
