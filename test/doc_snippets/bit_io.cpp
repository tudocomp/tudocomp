/**
 *
 * This file contains code snippets from the documentation as a reference.
 *
 * Please do not change this file unless you change the corresponding snippets
 * in the documentation as well!
 *
**/

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/io.hpp>

using namespace tdc;

const std::string example_text("This is the input text");

TEST(doc_bit_io, bit_output) {
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

    ASSERT_EQ(7U, ss.str().length());
}

TEST(doc_bit_io, bit_input) {
    Input input(example_text); // Create an Input from a string literal

    BitIStream ibits(input); // construct the bitwise input stream

    bool bit = ibits.read_bit(); // read a single bit

    uint8_t  a = ibits.read_int<uint8_t>(5); // read a 5-bit integer into a uint8_t
    uint16_t b = ibits.read_int<uint16_t>(); // read a 16-bit integer

    ASSERT_EQ(0, bit);
    ASSERT_EQ(21, a);
    ASSERT_EQ(6682, b);
}

