@page io Input and Output

# Input and Output

tudocomp provides abstractions for handling input from different kinds of
sources and output to different kinds of targets. Both hide the actual source or
target of the data -- e.g., a file or memory buffer.

## Input Handling

An Input (@ref tdc::Input) can be created from different data sources:

* a memory pointer (e.g., a string literal),
* a byte buffer (`std::vector<uint8_t>`),
* a file or
* an input stream (`std::istream`)[^direct-streaming].

For each type of data source, the Input class provides a corresponding
constructor:

@code{.cpp}
// Create an Input from a string literal
Input input_from_string("This is the input text");

// Create an Input from a given byte buffer (std::vector<uint8_t>)
Input input_from_buffer(buffer);

// Create an Input from a file
Input input_from_file(Path("example.txt"));

// Create an Input from a given std::istream
Input input_from_stream(std::cin); // from stdin
@endcode

[^direct-streaming]: Currently, direct streaming from an `std::istream` is not
supported. When an `Input` is constructed from an `istream`, the stream is fully
read and buffered in memory. This is an implementation decision that may change
in the future. Note that files, on the other hand, are not buffered and will
always be streamed from disk directly.

The input can be accessed in two conceptually different ways:

1. As a *stream*, so that bytes are read sequentially from the input source
   (the concept of online algorithms) or
2. as a *view*, providing random access to the input source like to an array of
   bytes (the concept of offline algorithms).

The choice is done by acquiring the respective handle using either the
@ref tdc::Input::as_stream or the @ref tdc::Input::as_view function.
The stream object returned by `as_stream` conforms to the `std::istream`
interface and also provides iterator access. The following code snippet
demonstrates using a given input as a stream:

@code{.cpp}
auto istream = input.as_stream(); // retrieve an input stream

// read the input character-wise using a range-based for loop
for(uliteral_t c : istream) {
    // ...
}
@endcode

The @ref tdc::View object returned by `as_view` provides the indexed access
`[]` operator and a `size` function to return the amount of bytes available
on the input.  The following code snippet demonstrates using an input as a view:

@code{.cpp}
auto iview = input.as_view(); //retrieve an input view

// compare the view's content against a certain string
ASSERT_EQ("foobar", iview);

// create a sub-view for a range within the main view
auto sub_view = iview.substr(1, 5);

ASSERT_EQ("ooba", sub_view); // assertion for the sub-view's contents

// iterate over the whole view character-wise in reverse order
for (len_t i = iview.size(); i > 0; i--) {
    uliteral_t c = iview[i-1];
    // ...
}
@endcode

The functions `as_stream` and `as_view` can be used multiple times to create
multiple streams or views on the same input, e.g., in case the input is to be
streamed more than once.

## Output Handling

An Output (@ref tdc::Output) can be created for different data sinks:

* a byte buffer (`std::vector<uint8_t>`),
* a file or
* an output stream (`std::ostream`).

Like for inputs, constructors for each type of sink are provided:

@code{.cpp}
// Create an Output to a given byte buffer (std::vector<uint8_t>)
Output output_to_buffer(buffer);

// Create an Output to a file:
Output output_to_file1(Path("example.txt"), false); // do not overwrite if exists (default)
Output output_to_file2(Path("example.txt"), true); // overwrite if exists

// Create an Output to a given std::ostream
Output output_to_stream(std::cout); // to stdout
@endcode

An output has to be generated sequentially and thus only provides a stream
interface via the @ref tdc::Output::as_stream function. The following code
snippet demonstrates this by copying an entire input to an output byte by
byte:

@code{.cpp}
auto istream = input.as_stream(); // retrieve the input stream
auto ostream = output.as_stream(); // retrieve the output stream

// copy the input to the output character by character
for(uliteral_t c : istream) {
    ostream << c;
}
@endcode

## Advanced I/O

The following pages cover advanced I/O functionality:
* @subpage bitio
* @subpage transform
