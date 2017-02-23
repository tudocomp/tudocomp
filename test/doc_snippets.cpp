/**
 *
 * This file contains the code snippets from the documentation as a reference.
 *
 * Please do not change this file unless you change the corresponding snippets
 * in the documentation as well!
 *
**/

#include <glog/logging.h>
#include <gtest/gtest.h>

#define SNIPPET(name) TEST(DocSnippets, name)

#include <tudocomp/tudocomp.hpp>

using namespace tdc;

const std::string example_text("This is the input text");

SNIPPET(input_stream) {
    // Create an Input from a string literal
    Input input(example_text);

    auto istream = input.as_stream(); // retrieve an input stream
    /*auto istream2 = istream; // create a second stream as a "rewind" position*/

    // read the input character-wise using a C++11 range-based for loop
    len_t i = 0;
    for(uliteral_t c : istream) {
        ASSERT_EQ(example_text[i++], c);
    }

    /*// read the input character-wise using the std::istream interface
    LOG(INFO) << "read like std::istream:"
    char c; i = 0;
    while(istream2.get(c)) {
        ASSERT_EQ(example_text[i++], c);
    }*/
}

SNIPPET(input_view) {
    // Create an Input from a string literal
    Input input(example_text);

    auto iview = input.as_view(); //retrieve an input view
    /*auto iview2 = iview; // create a shallow copy of the view*/

    // compare the view's content against a certain string
    ASSERT_EQ(example_text, iview);

    // create a sub-view for a range within the main view
    auto sub_view = iview.substr(1, 5);

    ASSERT_EQ(example_text.substr(1, 5), sub_view); // assertion for the sub-view's contents

    // iterate over the whole view character-wise in reverse order
    for (len_t i = iview.size(); i > 0; i--) {
        uliteral_t c = iview[i-1];
        ASSERT_EQ(example_text[i-1], c);
    }
}

SNIPPET(output_stream) {
    // Create an Input from a string literal
    Input input(example_text);

    // Create an Output to a string stream
    std::stringstream ss;
    Output output(ss);

    auto istream = input.as_stream(); // retrieve the input stream
    auto ostream = output.as_stream(); // retrieve the output stream

    // copy the input to the output character by character
    for(uliteral_t c : istream) {
        ostream << c;
    }

    ASSERT_EQ(example_text, ss.str());
}

SNIPPET(bit_output) {
    // Create an Output to a string stream
    std::stringstream ss;
    Output output(ss);

    {
    BitOStream obits(output); //construct the bitwise output stream

    obits.write_bit(0);     // write a single unset bit
    obits.write_bit(1);     // write a single set bit
    obits.write_int(27, 5); // write the value 27 using 5 bits (11011)
    obits.write_int(27, 3); // write the value 27 using 3 bits (truncated to 011)

    int a = 27;
    obits.write_int(a); // write the value 27 using 8*sizeof(int) bits (32)
                        // (00000000000000000000000000011011)

    uint8_t b = 27;
    obits.write_int(b); // write the value 27 using 8*sizeof(uint8_t) bits (8)
                        // (00011011)

    } // end of scope, write EOF sequence and destroy bit output stream

    ASSERT_EQ(7, ss.str().length());
}

SNIPPET(bit_input) {
    // Create an Input from a string literal
    Input input(example_text);

    BitIStream ibits(input); // construct the bitwise input stream

    bool bit = ibits.read_bit(); // read a single bit

    uint8_t  a = ibits.read_int<uint8_t>(5); // read a 5-bit integer into a uint8_t
    uint16_t b = ibits.read_int<uint16_t>(); // read a 16-bit integer

    ASSERT_EQ(0, bit);
    ASSERT_EQ(21, a);
    ASSERT_EQ(6682, b);
}

SNIPPET(iv_static) {
    // reserve a vector of 32 4-bit integers (initialized as zero)
    IntVector<uint_t<4>> iv4(32);

    // fill it with increasing values (0, 1, 2, ...)
    std::iota(iv4.begin(), iv4.end(), 0);

    // print size in bits
    ASSERT_EQ(128, iv4.bit_size());

    // demonstrate overflow
    ASSERT_EQ(iv4[0], iv4[16]);

    // reserve an additional bit vector with 32 entries
    BitVector bv(32);

    // mark all multiples of 3
    for(len_t i = 0; i < 32; i++) bv[i] = ((iv4[i] % 3) == 0);

    ASSERT_EQ(1UL, bv[0]);
    ASSERT_EQ(0UL, bv[7]);
    ASSERT_EQ(1UL, bv[15]);
}

SNIPPET(iv_dynamic) {
    // reserve a vector for 20 integer values (initialized as zero)
    // default to a width of 32 bits per value
    DynamicIntVector fib(20, 0, 32);

    ASSERT_EQ(640, fib.bit_size());

    // fill it with the Fibonacci sequence
    fib[0] = 0;
    fib[1] = 1;
    for(len_t i = 2; i < fib.size(); i++)
      fib[i] = fib[i - 2] + fib[i - 1];

    // find the amount of bits required to store the last (and largest) value
    auto max_bits = bits_for(fib.back());

    // bit-compress the vector to the amount of bits required
    fib.width(max_bits);
    fib.shrink_to_fit();

    ASSERT_EQ(260, fib.bit_size());
}

SNIPPET(algorithm_impl) {
    class MyAlgorithm : public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("undisclosed", "my_algorithm", "An example algorithm");
            m.option("param1").dynamic("default_value");
            m.option("numeric").dynamic(147);
            return m;
        }

        inline MyAlgorithm(Env&& env) : Algorithm(std::move(env)) {
        }

        inline void execute() {
            auto param1 = env().option("param1").as_string();
            auto numeric = env().option("numeric").as_integer();

            ASSERT_EQ("default_value", param1);
            ASSERT_EQ(777, numeric);
        }
    };

    auto my_algo = create_algo<MyAlgorithm>("numeric=777");
    my_algo.execute();
}

