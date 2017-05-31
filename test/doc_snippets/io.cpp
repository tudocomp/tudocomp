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

TEST(doc_io, input_stream) {
    // Create an Input from a string literal
    Input input(example_text);

    auto istream = input.as_stream(); // retrieve an input stream

    // read the input character-wise using a C++11 range-based for loop
    index_fast_t i = 0;
    for(uliteral_t c : istream) {
        ASSERT_EQ(example_text[i++], c);
    }
}

TEST(doc_io, input_view) {
    // Create an Input from a string literal
    Input input(example_text);

    auto iview = input.as_view(); //retrieve an input view

    // compare the view's content against a certain string
    ASSERT_EQ(example_text, iview);

    // create a sub-view for a range within the main view
    auto sub_view = iview.substr(1, 5);

    ASSERT_EQ(example_text.substr(1, 5), sub_view); // assertion for the sub-view's contents

    // iterate over the whole view character-wise in reverse order
    for (index_fast_t i = iview.size(); i > 0; i--) {
        uliteral_t c = iview[i-1];
        ASSERT_EQ(example_text[i-1], c);
    }
}

TEST(doc_io, output_stream) {
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

