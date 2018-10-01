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

#include <tudocomp/Compressor.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/EliasDeltaCoder.hpp>

#include "../test/util.hpp"

using namespace tdc;

// Implement a simple compressor
template<typename coder_t>
class MyCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "my_compressor", "An example compressor");
        m.param("coder").strategy<coder_t>(Coder::type_desc());
        return m;
    }

    using Compressor::Compressor;

    virtual void compress(Input& input, Output& output) override {
        // retrieve random access on the input
        auto view = input.as_view();

        // find the lexicographically smallest and largest characters
        uliteral_t c_min = ULITERAL_MAX;
        uliteral_t c_max = 0;

        for(uliteral_t c : view) {
            c_min = std::min(c_min, c);
            c_max = std::max(c_max, c);
        }

        // instantiate the encoder using the whole input alphabet
        typename coder_t::Encoder coder(
            config().sub_config("coder"), output, ViewLiterals(view));

        // encode the smallest and largest characters
        coder.encode(c_min, uliteral_r);
        coder.encode(c_max, uliteral_r);

        // define the range for all occuring characters
        Range occ_r(c_max - c_min);

        // encode text
        for(uliteral_t c : view) {
            coder.encode(c - c_min, occ_r);
        }
    }

    virtual void decompress(Input& input, Output& output) override {
        // retrieve an output stream
        auto ostream = output.as_stream();

        // instantiate the decoder using the whole input alphabet
        typename coder_t::Decoder decoder(
            config().sub_config("coder"), input);

        // encode the smallest and largest characters
        auto c_min = decoder.template decode<uliteral_t>(uliteral_r);
        auto c_max = decoder.template decode<uliteral_t>(uliteral_r);

        // define the range for all occuring characters
        Range occ_r(c_max - c_min);

        // decode text
        while(!decoder.eof()) {
            uliteral_t c = c_min + decoder.template decode<uliteral_t>(occ_r);
            ostream << c;
        }
    }
};

TEST(doc_compressor_impl, cycle) {
    const std::string example = "aaabbaabab";

    // Run compression cycles using different encoders
    test::roundtrip<MyCompressor<ASCIICoder>>(example);
    test::roundtrip<MyCompressor<BitCoder>>(example);
    test::roundtrip<MyCompressor<EliasDeltaCoder>>(example);
}

TEST(doc_compressor_impl, helpers) {
    // perform border case compression tests using different encoders
    test::roundtrip_batch(test::roundtrip<MyCompressor<ASCIICoder>>);
    test::roundtrip_batch(test::roundtrip<MyCompressor<BitCoder>>);
    test::roundtrip_batch(test::roundtrip<MyCompressor<EliasDeltaCoder>>);

    // perform compression tests on generated strings using different encoders
    test::on_string_generators(test::roundtrip<MyCompressor<EliasDeltaCoder>>, 15);
    test::on_string_generators(test::roundtrip<MyCompressor<BitCoder>>, 15);
    test::on_string_generators(test::roundtrip<MyCompressor<ASCIICoder>>, 15);
}

