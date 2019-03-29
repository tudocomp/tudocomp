@page compressor Compressors

# Compressors
Compressors transform an input into an output that can be losslessly restored
to the original input. What separates them from Coders is that compressors may
have random access on the entire input or scan it multiple times, and use
this to find information on how to achieve compression.

On a code level, the abstract class @ref tdc::Compressor serves as the
foundation for compression algorithm implementations. It defines the virtual
function @ref tdc::Compressor::compress, which is where the Compressor performs
its actuall compression. Apart from that, it also defines the
@ref tdc::Compressor::decompressor function, which returns an instance of a
decompressor that is able to decode and decompress the output created by the
`compress` function so that the decompressor's output matches the compressor's
original input. Decompressors (@ref tdc::Decompressor) therefore define the
@ref tdc::Decompressor::decompress function with the same signature as
`compress`, receiving the compressed input to create the decompressed output.

Compressors and Decompressors are separated to reduce template parameter
complexity. In many cases, compressors need more or at least different
information than decompressors, which often only need to decode a specific
format. For example, there are myriad strategies to achieve LZ77 compression
(sliding windows, hashing, suffix and LCP array, ...), but the output is always
an LZ77 factorization. An LZ77 decompressor needs not to know what strategy was
used to produce the factorization. In practice, this means we can save a
C++ template parameter.

Both Compressor and Decompressor inherit from @ref tdc::Algorithm, ie., they
provide a meta information object, receive a configuration and can be
implemented in a modular way using strategies for certain sub-tasks
(see @ref algo for more information). Commonly, encoding is done using a
@ref tdc::Coder as a strategy and it is the compressor's task to provide the
necessary context information and output -- in other words, *what* to encode
and *how* to encode it. Naturally, the decompressor needs to decode the output
using a matching decoder, which can also be considered a strategy.

## Compression
We provide an example implementation for a simple compressor, which does the
following to achieve compression:
1. Determine the lexicographically smallest and largest characters `c_min` and
   `c_max` in the input text using a linear scan of the input.
2. Instantiate an encoder for the whole input alphabet (`ViewLiterals`).
3. Store `c_min` and `c_max` to the output by encoding them using the literal
   range `uliteral_r`.
4. Define a range that covers values from 0 to `c_max - c_min` as a hint for
   the encoder.
5. Encode only the difference `c - c_min` between each character `c` from the
   input and the smallest one, `c_min`, using the previously defined range.

@code{.cpp}
// Implement a simple compressor
template<typename coder_t>
class ExampleCompressor : public Compressor {
public:
    inline static Meta meta() {
        // build the meta information object
        Meta m(Compressor::type_desc(), "example", "An example compressor");

        // bind the coder_t template type to the "coder" parameter
        m.option("coder").templated<coder_t>("coder");

        return m;
    }

    // inherit constructors from the base
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
            config.sub_config("coder"), output, ViewLiterals(view));

        // encode the smallest and largest characters first
        coder.encode(c_min, uliteral_r);
        coder.encode(c_max, uliteral_r);

        // define the range for all occuring characters
        Range occ_r(c_max - c_min);

        // encode text
        for(uliteral_t c : view) {
            coder.encode(c - c_min, occ_r);
        }
    }

    virtual std::unique_ptr<Decompressor> decompressor() override {
        // instantiate the example decompressor (see below)
        return Algorithm::instance<ExampleDecompressor<coder_t>>();
    }
};
@endcode

This example shows the two typical major phases of a compression
routine:
1. Scan the input -- possibly multiple times -- and store information
   about how to compress the text. In this case, we determine the character
   range and store it using two integers.
2. The input is encoded to a compressed output using that
   information. In this case, we shrink the character representation by
   subtracting the range's minimum.

Note that this simple type of compression could as well be implemented as a
coder, because it only requires the input alphabet as a context (and that is
provided via the ViewLiterals iterator). The @ref tdc::SigmaCoder is a more
sophisticated variant of the example that also reduces the alphabet to only
those characters that actually occur in the input (the *effective alphabet*).

## Decompression
The following example decompresses the output produced by the
`ExampleCompressor`:
@code{.cpp}
// Decompressor for ExampleCompressor
template<typename coder_t>
class ExampleDecompressor : public Decompressor {
public:
    inline static Meta meta() {
        // build the meta information object
        Meta m(Decompressor::type_desc(), "example", "The example decompressor");

        // bind the coder_t template type to the "coder" parameter
        m.option("coder").templated<coder_t>("coder");

        return m;
    }

    // inherit constructors from the base
    using Decompressor::Decompressor;

    virtual void decompress(Input& input, Output& output) override {
        // retrieve an output stream
        auto ostream = output.as_stream();

        // instantiate the decoder for the input
        typename coder_t::Decoder decoder(config().sub_config("coder"), input);

        // decode the smallest and largest characters
        auto c_min = decoder.template decode<uliteral_t>(uliteral_r);
        auto c_max = decoder.template decode<uliteral_t>(uliteral_r);

        // define the range for all occuring characters
        Range occ_r(c_max - c_min);

        // decode the text
        while(!decoder.eof()) {
            // decode literal
            uliteral_t c = c_min + decoder.template decode<uliteral_t>(occ_r);

            // output to stream
            ostream << c;
        }
    }
};
@endcode

#### Wrapping decompression into the compressor.
For legacy reasons mainly, it is possible to wrap the decompression routine
into the compressor class. This can be used for reduced code bloating if the
compressor and the decompressor share the same template parameters, like in our
example.

Alternatively to inheriting from @ref tdc::Compressor, we could inherit from
@ref tdc::CompressorAndDecompressor. The @ref tdc::WrapDecompressor is then
used to convert the compressor to a decompressor:
@code{.cpp}
inline std::unique_ptr<Decompressor> decompressor() const override {
    return std::make_unique<WrapDecompressor>(*this);
}
@endcode
