# Abstract

The **T**echnical **U**niversity of **DO**rtmund **COMP**ression Framework,
*tudocomp*, is a lossless compression framework with the aim to support and
facilitate the implementation of novel compression algorithms. It already
comprises a range of standard data compression and encoding algorithms. These
can be mixed and parameterized with the following uses in mind:

* Baseline implementations of well-known compression schemes.
* Detailed benchmarking and comparison of compression and encoding algorithms.
* Easy integration of new algorithm implementations.

# Philosophy

The framework offers a solid and extensible base for new implementations. It is
designed so that most individual processes are modularized and interchangeable.
This way, the user can mix and match algorithms to find the optimal compression
strategy for a given input. The framework gives this opportunity while creating
as little performance overhead as possible.

## Compressors and Coders

*Compressors*, in terms of this framework, transform an input sequence (of
bytes or characters) and write the result to an output. Each compressor is also
required to implement a *decompressor* that can restore the original input
losslessly from the compressed output. Apart from this, there are no strict
rules as to *what* kind of transformation of the input occurs.

*Coders* serve for encoding primitve data types into a bit sequence with limited
context (contrary to compressors, which may randomly access the entire
input sequence). To this end, coders conceptually form the end of a compression
chain by producing the final output bit sequence. A coder may receive additional
information by a compressor in order to be able to encode data more efficiently
(e.g. value ranges for bit-optimal encoding or even an alphabet distribution for
low-entropy encoding). Analogously to compressors, every coder must be
accompanied by a matching *decoder*.

By design, a *coder* can also be used as a compressor.

## Modularity

Compressors and coders form a subset of our concept of *algorithms*. Any
non-trivial type to be exposed to users by *tudocomp* shall be seen as an
algorithm.

Algorithms shall be implemented in a *modular* way. Modules shall be
interchangeable, so any task or subtask may be approached using different
strategies.

For instance, picture a compression algorithm that consists of more than one
step to achieve its compression or decompression. If each step could be seen as
a module that may solve its sub-task in different ways, it qualifies for
modularization.

To achieve this kind of modularity, the use of C++'s meta programming features
(namely templates) is heavily encouraged. *tudocomp* is designed in a way that
allows template parameters to be populated seemingly at runtime.

## Compression Chains

Compressors and coders can be chained so that the output of one becomes the
input of another.

## Library and Command-Line

The framework consists of two major components: the compression *library*
(often used synonymously with *tudocomp* in this document) and the command-line
application (`tdc`).

The library contains the core interfaces and provides implementations for
various compression algorithms. It is fully functional for use in third-party
applications.

Using `tdc` (the commmand-line application), *tudocomp*'s implementations can
be invoked from a shell.

The interface between *tudocomp* and `tdc` is the *registry*. Every registered
algorithm is exposed to `tdc` with a unique identifier specified by its
implementation.

# Features

*tudocomp* includes the following set of features:

* Flexible template-based interface for algorithms (e.g. compressors or integer
  encoders)
    * Registry for easy exposition to the `tdc` command-line tool
    * Definition of parameters that can be passed (e.g. via the command-line)
    * On-the-fly population of template parameters for easy exchangability
      of strategies and other modules (also via the command-line)
* Simple interface for compressors
* Powerful interface for integer encoders
    * Possibility to receive context information from compressors, such as
      integer value ranges or the input alphabet, for more efficient encoding
      capabilities
* Arbitrary-width integer vectors and bit vectors
* Bitwise I/O
* Measurement of running time and heap allocation in freely definable and
  nestable code phases
    * `malloc` override
    * Export of statistics to JSON
    * Visualization of exported statistics in a web application
    * Export of charts to `png` or `svg`
      ([InkScape](https://inkscape.org/)-compatible and LaTeX-friendly)
* Implementations of text data structures, including
    * Suffix array (using `divsufsort`) and inverse
    * LCP array and its pre-stages (Phi array and permuted LCP)
    * Burrows-Wheeler transform and LF table
    * Optional bit-compression either during or after construction
* Implementations of various integer encoders, including:
    * Binary and unary encoding
    * Elias-Gamma and -Delta encoding
    * VByte coding
    * Huffman coding
    * Human-readable ASCII representation for debugging purposes
    * Custom static low-entropy encoding (SLE)
* Implementations of various compression algorithms, including:
    * LZ77 using a sliding window or the LCP array
    * LZ78 with exchangeable trie structure
    * Run-length encoding
    * Custom variants of LZ77 (lcpcomp) and LZ78 (LZ78U)
* Utilities for swift unit test implementation
* String generators for testing and benchmarking purposes
    * Random strings with uniform character distribution
    * Fibonacci words
    * Thue-Morse strings
    * Run-Rich strings (Matsubara et al.)
* Scripts for downloading a selected
  [text corpus collection](http://tudocomp.org/text-collection.html) for testing
  and benchmarking purposes
* Tools to compare running time, heap memory usage and compression ratio of
  different compressor suites (*tudocomp* as well as third-party) on a freely
  definable set of inputs

# Usage

This section describes the usage of *tudocomp* as a `C++` library and as a
command-line tool, respectively.

## Building

The framework is built using [CMake](https://cmake.org) (3.0.2 or later).
It is written in `C++11` with GNU extensions and has been tested with the `gcc`
compiler family (version 4.9.2 or later) and `clang` (version 3.5.2 or later).
The build process requires a [Python](https://www.python.org/) interpreter
(3 or later) to be installed on the system (`py` scripts are invoked directly).

When these requirements are met, the following chain of commands on a clean
clone of the project suffices to build *tudocomp*:

~~~
$ mkdir build && cd build
$ cmake ..
$ make
~~~

### Dependencies

*tudocomp* has the following external dependencies:

* [SDSL](https://github.com/simongog/sdsl-lite)
  (2.1 or later).
* [Gflags](https://gflags.github.io/gflags) (2.1.2 or later).
* [Google Logging (glog)](https://github.com/google/glog) (0.34 or later).

Additionally, the tests require
[Google Test](https://github.com/google/googletest) (1.7.0 or later).

The CMake build process will either find the external dependencies on the build
system if they have been properly installed, or automatically download and build
them from their official repositories in case they cannot be found.

For building the documentation, the following tools are required:

* [LaTeX](http://www.latex-project.org) (specifically the `pdflatex` component)
* [Doxygen](http://doxygen.org) (1.8 or later).
* [Pandoc](http://pandoc.org) (1.16 or later).

### Windows Support

We highly recommend users of Windows 10 or later to use the
[Bash on Ubuntu on Windows](https://msdn.microsoft.com/en-us/commandline/wsl/about).

That being said, *tudocomp* has no explicit support for Windows. However, the
project can be built in a [Cygwin](https://www.cygwin.com/) environment with a
limited feature set. Cygwin does not allow overrides of `malloc`, therefore the
heap allocation counter cannot work and statistics tracking becomes largely
nonfunctional.

## Command-line Tool

The executable `tdc` is the command-line application that bundles all
compression, encoding and other algorithms contained in *tudocomp*. It provides
a fast and easy way to compress and decompress a file with a specified
compressor or chain of compressors.

It contains a common help output that can be accessed by passing `--help`.
Furthermore, the aforementioned set of algorithms that are available for use
via the command-line can be listed by passing `--list`.

The following are typical ways to use `tdc` and should give an idea about how to
use it. The example is using a simple LZ77 compressor and human readable
encoding.

Compress `file.txt`, write to `file.txt.tdc`:
: `$ tdc -a "lzss(coder=ascii)" file.txt`

Compress to a specific output file, overwrite if it exists:
: `$ tdc -a "lzss(coder=ascii)" file.txt -fo out.tdc`

Decompress a compressed file, print to stdout:
: `$ tdc -d out.tdc --usestdout`

Decompress using a specific decompressor, print to stdout:
: `$ tdc -d -a "encode(coder=ascii)" --stdout`

Print the 10^th^ Fibonacci word to stdout:
: `$ tdc -g "fib(10)" --usestdout`

Compress the 10^th^ Fibonacci word, print to stdout without header:
: `$ tdc -g "fib(10)" -a "lzss(coder=ascii)" --raw --usestdout`

## Library

The library part of *tudocomp* is generated as the `libtudocomp_algorithms.a`
artifact that can be used for static linking. Using this and the headers
(located in the `include` directory tree), *tudocomp* can be used as an external
library in third-party applications.

The [Doxygen documentation](@URL_DOXYGEN@) provides an overview of the
framework's full API, including the contained compression and encoding
implementations.

## License

The framework is published under the
[Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).

# Tutorial

This section provides a guided tour through some of the core features of
*tudocomp*.

Please note that more detailed information about the use of specific
classes or methods can be found in the [Doxygen documentation](@URL_DOXYGEN@).

## Input and Output

*tudocomp* provides an abstraction for handling input from different kinds of
sources and output to different kinds of targets. These are straightforwardly
named  [`Input`](@DX_INPUT@) and [`Output`](@DX_OUTPUT@). Both
hide the actual source or target of the data (e.g. a file or memory buffer).

This section will describe their usage briefly along with some examples.

### Reading an Input

An [`Input`](@DX_INPUT@) can be created from different data sources:

* a memory pointer (e.g. a string literal),
* a byte buffer (`std::vector<uint8_t>`),
* a file or
* an input stream(`std::istream`)[^direct-streaming].

For each type of data source, the `Input` class provides a corresponding
constructor:

~~~ {.cpp}
// Create an Input from a string literal
Input input_from_memory("This is the input data");

// Create an Input from a given byte buffer (std::vector<uint8_t>)
Input input_from_buffer(buffer);

// Create an Input from a file
Input input_from_file(Input::Path{"example.txt"});

// Create an Input from a given std::istream
Input input_from_stream(std::cin); // from stdin
~~~

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
[`as_stream`](@DX_INPUT_ASSTREAM@) or the
[`as_view`](@DX_INPUT_ASVIEW@) function. The stream object returned by
`as_stream` conforms to the `std::istream` interface and also provides iterator
access. The object returned by `as_view` provides the indexed access `[]`
operator for and the function `size()` to return the amount of bytes available
on the input.

The following code snippet demonstrates using a given input as a view:

~~~ { .cpp }
auto istream = input.as_stream(); // retrieve an input stream
auto istream2 = istream; // create a second stream as a "rewind" position

// read the input character-wise using a C++11 range-based for loop
for(uliteral_t c : istream) {
    // ...
}

// read the input character-wise using the std::istream interface
char c;
while(istream2.get(c)) {
    // ...
}
~~~

Note how `istream2` is created as a copy of `istream`. This way, `istream2`
points at the same stream position as `istream` at the time the copy is created.
It can be used as a "rewind" point for use independently of `istream`.

The type [`uliteral_t`](@DX_ULITERAL_T@) is one of *tudocomp*'s core
types and shall be used for single characters.

In contrast, The following code snippet demonstrates using an input as a view:

~~~ { .cpp }
auto iview = input.as_view(); //retrieve an input view
auto iview2 = iview; // create a shallow copy of the view

// compare the view's content against a certain string
// the CHECK macro is Google Logging's "assert"
CHECK(iview == "foobar");

// create a sub-view for a range within the main view
auto sub_view = iview.substr(1, 5); 

CHECK(sub_view == "ooba"); // assertion for the sub-view's contents

// iterate over the whole view character-wise in reverse order
for (len_t i = iview.size() - 1; i >= 0; i--) {
    uliteral_t c = iview[i];
    // ...
}
~~~

Note that copies and sub-views are shallow, ie. they point to the same memory
location as the original view and thus have the same content.

The type [`len_t`](@DX_LEN_T@) is another one of *tudocomp*'s core
types and shall be used for lengths and indices.

### Producing an Output

An [`Output`](@DX_OUTPUT@) can be created for different data sinks:

* a byte buffer (`std::vector<uint8_t>`),
* a file or
* an output stream (`std::ostream`).

Like `Input`, it provides a constructor for each type of sink:

~~~ {.cpp}
// Create an Output to a given byte buffer (std::vector<uint8_t>)
Output output_to_buffer(buffer);

// Create an Output to a file:
Output output_to_file1("example.txt", false); // do not overwrite if exists (default)
Output output_to_file2("example.txt", true); // overwrite if exists

// Create an Output to a given std::ostream
Output output_to_stream(std::cout); // to stdout
~~~

An output has to be generated sequentially and thus only provides a stream
interface via the [`as_stream`](@DX_OUTPUT_ASSTREAM@) function. The
following code snippet demonstrates this by copying an entire input to an
output:

~~~ { .cpp }
auto istream = input.as_stream(); // retrieve the input stream
auto ostream = output.as_stream(); // retrieve the output stream

// copy the input to the output character by character
for(uliteral_t c : istream) {
    ostream << c;
}
~~~

### Bitwise I/O

The framework provides the classes [`BitIStream`](@DX_BITISTREAM@) and
[`BitOStream`](@DX_BITOSTREAM@) for bitwise input and output. They are
wrappers around `std::istream` and `std::ostream`, respectively, and provide
functionality to read or write single bits or fixed-width (MSBF order) integers
from their underlying stream.

Single bits are written using [`write_bit`](@DX_BITOSTREAM_WRITE_BIT@),
integers using [`write_int`](@DX_BITOSTREAM_WRITE_INT@).

The following example performs several bitwise write operations on an output:

~~~ {.cpp}
auto ostream = output.as_stream(); // retrieve an output stream
{
BitOStream obits(ostream); //construct the bitwise output stream

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
~~~

There is important logic in the [destructor](@DX_BITOSTREAM_DTOR@) of
`BitOStream`: Since the stream writes bits to an underlying byte stream, it
needs to write a few extra bits at the end of the stream in order to indicate
the end for an eventual bit input stream.

Note how [`write_int`](@DX_BITOSTREAM_WRITE_INT@) will use the default
size of the passed integer's type if no bit width is explicitly passed in the
second argument.

The following example performs several bitwise read operations from an input:

~~~ {.cpp}
auto istream = input.as_stream(); // retrieve an input stream
BitIStream ibits(istream); // construct the bitwise input stream

bool bit = ibits.read_bit(); // read a single bit

uint8_t  a = ibits.read_int<uint8_t>(5); // read a 5-bit integer into a uint8_t
uint16_t b = ibits.read_int<uint16_t>;   // read a 16-bit integer
~~~

Note how [`read_int`](@DX_BITISTREAM_READ_INT@) requires a template
parameter in order to "know" into which data type the read integer will be
stored and perform the respective conversion. If no bit width is given, the
default size of the data type will be used.

Beyond writing single bits and fixed-width integers, the bit I/O features some
basic integer encodings:

* Unary code ([`write_unary`](@DX_BITOSTREAM_WRITE_UNARY@) / 
  [`read_unary`](@DX_BITISTREAM_READ_UNARY@))
* Elias gamma code
  ([`write_elias_gamma`](@DX_BITOSTREAM_WRITE_GAMMA@) /
  [`read_elias_gamma`](@DX_BITISTREAM_READ_GAMMA@))
* Elias delta code
  ([`write_elias_delta`](@DX_BITOSTREAM_WRITE_DELTA@) /
  [`read_elias_delta`](@DX_BITISTREAM_READ_DELTA@))
* Compressed integers
  ([`write_compressed_int`](@DX_BITOSTREAM_WRITE_VBYTE@) /
  [`read_compressed_int`](@DX_BITISTREAM_READ_VBYTE@))

# Old Tutorial

>> *TODO*: This will probably disappear entirely.

## A Simple Compressor

This section presents the steps necessary to implement a simple compressor. The
implementation that is developed here is available in the framework's
repository in the `/include/tudocomp/example/` directory.

### Implementing the Compressor interface

Any compressor needs to implement the [`Compressor`](@DX_COMPRESSOR@)
interface. A complete implementation consists of

* a constructor accepting an rvalue reference to an environment
  (`Env&&`),
* implementations of the `compress` and `decompress` functions (so that the
  output of `compress`, when passed as the input of `decompress`, will be
  transformed back to the original input of `compress`),
* a static function `meta()` that yields a `Meta` information
  object about the compressor.

Note that while the latter (`meta()`) is not strictly defined in the
`Compressor` class, it is required due to the nature of templated construction.

The class [`Env`](@DX_ENV@) represents the compressor's runtime environment.
It provides access to runtime options as well as the framework's statistics
tracking functionality. A compressor conceptually owns[^cpp11-ownership] its
environment, therefore the constructor takes an rvalue reference to it. The
reference should always be delegated down to the base constructor
(using `std::move`).

[^cpp11-ownership]: This refers to the C++11 ownership semantics, ie. a
`unique_ptr<Env>` is stored internally.

A [`Meta`](@DX_META@) object contains information about an algorithm
(e.g. compressors) such as its name and type. This information is used by the
generic algorithm constructor `create_algo`, which will be explained below, as
well as for the registry of the command-line application.

The following example header (`/include/tudocomp/example/ExampleCompressor.hpp`)
contains a minimal `Compressor` implementation named `ExampleCompressor`:

~~~ { .cpp }
// [/include/tudocomp/example/ExampleCompressor.hpp]

#ifndef _INCLUDED_EXAMPLE_COMPRESSOR_HPP_
#define _INCLUDED_EXAMPLE_COMPRESSOR_HPP_

#include <tudocomp/tudocomp.hpp>

namespace tdc {

class ExampleCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "example_compressor",
               "This is an example compressor.");

        return m;
    }

    inline ExampleCompressor(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
    }

    inline virtual void decompress(Input& input, Output& output) override {
    }
};

}

#endif
~~~

The [`tdc`](@DX_TDC@) namespace contains most of the core types
required for implementing compressors, including the `Compressor` interface
and the `Env` and `Meta` types.

The `Meta` object returned by `meta()` contains the following information:

* The algorithm type (in this case, a `"compressor"`),
* the algorithm's identifier (for shell compatibility, this should not contain
  any spaces or special characters) and
* a brief description of the algorithm (which is be displayed in the
  command-line help output).

### Example: Run-Length Encoding

The following example implements the `compress` method so that it yields a
run-length encoding of the input. In run-length encoding, sequences (runs)
of the same character are replaced by one single occurence, followed by the
length of the run.

For example, the input `"abcccccccde"`, which contains a run of seven `c`
characters, is encoded as `"abc%6%de"`, where `%6%` designates that the
preceding character is repeated six times. This way, the original input can be
restored from the encoded string.

~~~ { .cpp }
inline virtual void compress(Input& input, Output& output) override {
    auto istream = input.as_stream(); // retrieve the input stream
    auto ostream = output.as_stream(); // retrieve the output stream

    char current; // stores the current character read from the input
    char last; //stores the character that preceded the current character
    size_t counter = 0; // counts the length of the run of the current character

    // writes the current run to the output stream
    auto emit_run = [&]() {
        if (counter > 3) {
            // if the run exceeds 3 characters, encode the run using the %% syntax
            ostream << last << '%' << counter << '%';
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
~~~

The decoding is handled by the `decompress` method as follows. It attempts to
find patterns of the form `c%n%` and writes the character `c` to the output
`n` times. Any character not part of such a pattern will simply be copied to
the output.

~~~ { .cpp }
inline virtual void decompress(Input& input, Output& output) override {
    auto iview = input.as_view(); // retrieve the input as a view (merely for educational reasons)
    auto ostream = output.as_stream(); // retrieve the output stream

    char last = '?'; // stores the last character read before a "%n%" pattern is encountered

    // process the input
    for (size_t i = 0; i < iview.size(); i++) {
        if (iview[i] == '%') {
            // after encountering the '%' chracter, parse the following characters
            // as a decimal number until the next '%' is encountered
            size_t counter = 0;
            for (i++; i < iview.size(); i++) {
                if (iview[i] == '%') {
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
~~~

Note that this implementation will obviously not work if the original input
contained the `%` character. It merely serves as an example.

> *Exercise*: Implement the `decompress` method using the input as stream
rather than a view.

### Magic

In order to identify what compressor has been used to produce a compressed
output, the driver application can prepend a unique identifier
(*magic keyword*) to the output.

It is important to note that this is *not* the responsibility of the compressor.

## Unit Tests

This section provides a guide to implementing unit tests for the framework.
Unit testing is done with the aid of the
[Google Test](https://github.com/google/googletest) library. The test sources
are located in the `test` directory in the repository root.

### Registering and Running unit tests

The test source files categorize the unit tests into test suites and are
registered in the `CMakeLists.txt` file.

The generated Makefile contains a target for registered each test suite.
For example, `make tudocomp_tests` invokes the `tudocomp_tests` suite which is
contained in `tudocomp_tests.cpp`. The `check` target executes all registered
test suites in succession.

The `sandbox_tests` suite is ignored by the framework's repository and can be
used for quick developmental tests to avoid the registration procedure.

A test suite consists of including the *Google Test* library and at least one
unit test as follows, starting with an empty `example_tests.cpp`

~~~ { .cpp }
// [/test/example_tests.cpp]

#include <gtest/gtest.h>

TEST(example, test) {
    ASSERT_TRUE(true);
}

~~~

The corresponding registration line in `CMakeLists.txt` looks as follows:

~~~
run_test(example_tests DEPS ${BASIC_DEPS})
~~~

The Makefile generated by CMake will contain a target `example_tests` that
executes the example test suite.

### Implementing unit tests

The following snippet provides a simple unit test for the `ExampleCompressor`
from the previous section, the run-length encoder:

~~~ { .cpp }
// [/test/example_tests.cpp]

#include <gtest/gtest.h>

#include <tudocomp/example/ExampleCompressor.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

TEST(example, compress) {
    // instantiate the ExampleCompressor
    auto compressor = tdc::create_algo<tdc::ExampleCompressor>();

    // create the input for the test (a string constant)
    tdc::Input input("abcccccccde");

    // create the output for the test (a buffer)
    std::vector<uint8_t> buffer;
    tdc::Output output(buffer);

    // invoke the compress method
    compressor.compress(input, output);

    // retrieve the output as a string
    std::string output_str(buffer.begin(), buffer.end());

    // compare the expected result against the output string to determine test failure or success
    ASSERT_EQ("abc%6%de", output_str);
}
~~~

*tudocomp* provides the [`create_algo`](@DX_CREATEALGO@) function
template that properly instantiates compressors (or more precisely: any class
inheriting from [`Algorithm`](@DX_ALGORITHM@)). In this example, the
compressor's input is created from a string constant and the output is linked to
a byte buffer that will be filled. After invoking `compress`, the output is
tested against the expected result.

>> *TODO*: I would like to rename `create_algo` to something memorable like
           `instantiate`.

>> *TODO*: Once we have string generators (random, Markov, ...), they should
be mentioned here.

A typical compression test case should also test that `decompress` restores
the original input when fed the output of `compress`. *tudocomp* provides
a helper function that performs such a compression cycle, which is presented
in the following unit test:

~~~ { .cpp }
#include "tudocomp_test_util.hpp"

TEST(example, roundtrip) {
    test::roundtrip<tdc::ExampleCompressor>("abcccccccde", "abc%6%de");
}
~~~

>> *TODO*: `test::roundtrip` is not in Doxygen!

[`test::roundtrip`](about:blank) performs the following operations: It

1. instantiates the given compressor type (ie. `ExampleCompressor`),
2. passes the input string (ie. `"abcccccccde"`) to the `compress` method,
3. tests the compression result against the second string (ie. `abc%6%de`),
4. passes the compression result to the `decompress` method and
5. tests the decompression result against the input string.

This way, it spans an entire compression cycle. Alternatively, since often the
compression result is not an ASCII string, the expected compression result can
be passed as a vector of bytes like so:

~~~ { .cpp }
TEST(example, roundtrip_bytes) {
    std::vector<uint8_t> v { 97, 98, 99, 37, 54, 37, 100, 101 };
    test::roundtrip<tdc::ExampleCompressor>("abcccccccde", v);
}
~~~

>> *TODO*: We need to consider if the term *roundtrip* should be replaced by
   something more meaningful.

## Runtime Statistics

*tudocomp* provides functionality to measure the running time and the peak
amount of dynamically allocated memory (e.h. via `malloc` or `new`) over the
course of a compression or decompression run.

This functionality is accessible via a compressor's *environment* (represented
by the [`Env`](@DX_ENV@) class), which can be retrieved using the
[`env()`](@DX_ALGORITH_ENV@) function.

Runtime statistics are tracked in *phases*, ie. the running time and memory
peak can be measured for individual stages during a compression run. These
phases may be nested, ie. a phase can consist of multiple other phases. When
a compressor is instantiated (using `create_algo`) it will automatically enter
a *root phase*.

The measured data can be retrieved as JSON for visualization in the
[*tudocomp Charter*](#charter-web-application) or processing in a third-party application.

> *Note:* In a *Cygwin* environment, due to its nature of not allowing overrides
          of `malloc` and friends, memory allocation cannot be measured.

### Usage

Making use of the statistics tracking functions is as easy as sorrounding
single phases with calls to the
[`begin_stat_phase`](@DX_ENV_BEGINSTATPHASE@) and
[`end_stat_phase`](@DX_ENV_ENDSTATPHASE@) functions like so:

~~~ { .cpp }
env().begin_stat_phase("Phase 1");
    // ... Phase 1
env().end_stat_phase();
env().begin_stat_phase("Phase 2");
    // ... Phase 2
    env().begin_stat_phase("Phase 2.1");
        // ... Phase 2.1, part of Phase 2
    env().end_stat_phase();
    // ... Phase 2
    env().begin_stat_phase("Phase 2.2");
        // ... Phase 2.2, part of Phase 2
    env().end_stat_phase();
    // ... Phase 2
env().end_stat_phase();
~~~

> *Note*: In order to receive meaningful results, each phase should "clean up"
          properly, ie. it should free any memory that is no longer needed after
          the phase is finished.

During any phase, custom statistics can be logged using the
[`log_stat`](@DX_ENV_LOGSTAT@) method like so:

~~~ { .cpp }
env().log_stat("A statistic", 147);
env().log_stat("Another statistic", 0.5);
~~~

>> *TODO*: Currently, there are only overloads for integer types. Overloads
   for `bool` and `std::string` should be added at least.

Statistic tracking is concluded using the
[`finish_stats`](@DX_ENV_FINISHSTATS@) function, which yields a
reference to a [`Stat`](@DX_STAT@) object. The JSON can be written to a
stream or retrieved as a string using its [`to_json`](@DX_STAT_TOJSON@)
function overloads:

~~~ { .cpp }
// finish statistics
auto& stats = compressor.env().finish_stats();

// print JSON to a stream directly
stats.to_json(std::cout);

// retrieve JSON as string
std::string json = stats.to_json();
~~~

### Charter Web Application

The [tudocomp Charter](@URL_CHARTER@) is a JavaScript-based web application
that visualizes the statistics JSON output. Based on the data, it plots a
bar chart that displays the single phases of the compression run with their
running time on the X axis and their peak heap memory usage on the Y axis.

![A diagram plotted by the Charter.](media/charter_diagram.png)

The dashed line within a phase bar displays the *memory offset* of the phase,
ie. how much memory was already allocated at the beginning of the phase. Ergo,
the top border of the bar displays the global, application-wide memory peak
during the phase, while the phase's local memory peak is the difference between
the top and the dashed line.

This information is explicitly printed in table view below the diagram. This is
also where custom statistics are printed. The table view of a phase will also be
displayed as a tooltip when the mouse is moved over its bar in the diagram.

![The table view of a statistics phase.](media/charter_table.png)

The Charter provides several options to customize the chart, as well as
exporting it as either a vector graphic (`svg`) or an image file (`png`).

## Options

In *tudocomp*, algorithms can take runtime *options* that may alter their
behaviour (e.g. the minimum length of a run before it is replaced in the
run-length code example above). These options can be passed, for instance,
via the command-line when invoking the driver application.

The framework knows two different types of runtime options, *dynamic* options
and *templated* options. The options that an algorithm takes are defined in
its meta information object.

### Dynamic Options

Dynamic options are options that accept values of any type that can be parsed
from a string representation (because options passed from a command-line, for
instance, are generally in string representation). For primitive types such
as booleans or integers, parsers are predefined in the
[`OptionValue`](@DX_OPTIONVALUE@) class.

In the following examples, two dynamic options are introduced to the
[run-length encoder](#example-run-length-encoding) example in the `Compressor`'s
meta information object. The `minimum_run` option will determine the minimum
length of a run before it is encoded (3 by default), while the `rle_symbol`
option defines the separation symbol used to use for encoding runs.

~~~ {.cpp}
inline static Meta meta() {
    Meta m("compressor", "example_compressor",
           "This is an example compressor.");

    //Define options
    m.option("minimum_run").dynamic("3");
    m.option("rle_symbol").dynamic("%");

    return m;
}
~~~

The `Meta`'s [`option`](@DX_META_OPTION@) method introduces a new
option. Using [`dynamic`](@DX_OPTIONBUILDER_DYNAMIC@), this option
is declared a dynamic option with the specified default value. The default value
is used when the option's value was not explicitly passed (ie. via the command
line).

In the following snippet, these options are used to alter the actual run-length
encoding done by the `emit_run` function:

~~~ {.cpp}
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
~~~

Note how options are accessible via the environment's
[`option`](@DX_ENV_OPTION@) function, which returns the corresponding
[`OptionValue`](@DX_OPTIONVALUE@) object.

> *Exercise*: Modify the decompression of the run-length encoder so that it
              uses the `rle_symbol` option as well.

### Templated Options

Since algorithms are meant to be modular, the framework provides functionality
to pass a sub-algorithm as an option of the main algorithm, which the main
algorithm then instantiates at runtime. This is done using templated options.

The following example declares the main algorithm,
`TemplatedExampleCompressor`, with a template parameter `encoder_t` and the
corresponding templated option:

~~~ {.cpp}
template <typename encoder_t>
class TemplatedExampleCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "example_compressor",
               "This is an example compressor.");

        //Define options
        m.option("encoder").templated<encoder_t>();

        return m;
    }

    // ...
};
~~~

The following snippet shows an alternative implementation of the
[run-length encoder](#example-run-length-encoding)'s `compress` function using
the templated option:

~~~ {.cpp}
inline virtual void compress(Input& input, Output& output) override {
    auto istream = input.as_stream(); // retrieve the input stream
    auto ostream = output.as_stream(); // retrieve the output stream

    char current; // stores the current character read from the input
    char last; //stores the character that preceded the current character
    size_t counter = 0; // counts the length of the run of the current character

    // create the encoder
    encoder_t encoder(env().env_for_option("encoder"));

    // retrieve the first character on the stream
    if (istream.get(last)) {
        // continue reading from the stream
        while(istream.get(current)) {
            if (current == last) {
                // increase length of the current run
                counter++;
            } else {
                // emit the previous run
                encoder.emit_run(last, counter);

                // continue reading from the stream, starting a new run
                last = current;
                counter = 0;
            }
        }

        // emit the final run
        encoder.emit_run(last, counter);
    }
}
~~~

Note how in this example, the template type `encoder_t` is used to create an
encoder. The function `emit_run` has been removed, instead the type `encoder_t`
is expected to declare `emit_run` accepting two parameters: the character and
the length of the run. This way, the actual encoding has been made *modular*.

The function [`env_for_option`](@DX_ENV_ENVFOROPTION@) is used to
create an environment from the current environment's `"encoder"` option.
`encoder_t` accepts this in its constructor and get nested options from it.

For instance, `encoder_t` could accept the dynamic options `minimum_run` and
`rle_symbol` and encode a run like in the previous section. However, as long as
`encoder_t` provides the `emit_run` function, it could use any arbitrary
strategy to encode the run. In either scenario, the `decompress` method would
have to use the sub-algorithm as well in order to decode the runs correctly.

> *Exercise*: Implement an encoder `ExampleRunEmitter` for the
              `TemplatedExampleCompressor`.

> *Exercise*: Implement the `decompress` function for the
              `TemplatedExampleCompressor` using the given encoder type (hint:
               add a `decode_run` function to the encoder).

> *Example*: A full example of the `TemplatedExampleCompressor` is available in
             the `include/tudocomp/example` directory in the framework's
             repository. Most of the framework's compressor implementations
             follow this scheme and can be viewn as advanced examples as well.

### Unit Testing with Options

>> *TODO*: `test::roundtrip` is not in Doxygen!

In the [Unit Tests](#unit-tests) section, the [`test::roundtrip`](about:blank)
method was introduced for testing a compression cycle. In another overload,
additionally to the input and expected encoding, options can be passed as a
third argument like so:

~~~ {.cpp}
TEST(example, roundtrip_options) {
    using namespace tdc;

    // Test with options
    test::roundtrip<ExampleCompressor>("abcccccccde", "abc#6#de",  "minimum_run = '6', rle_symbol = '#'");
    test::roundtrip<ExampleCompressor>("abccccccde",  "abccccccde", "minimum_run = '6', rle_symbol = '#'");

    // Test defaults
    test::roundtrip<ExampleCompressor>("abcccccccde", "abc%6%de");

    // Test partially with defaults
    test::roundtrip<ExampleCompressor>("abcccccccde", "abc%6%de",  "minimum_run = '6'");
    test::roundtrip<ExampleCompressor>("abccccccde",  "abccccccde", "minimum_run = '6'");
    test::roundtrip<ExampleCompressor>("abccccde",    "abc#3#de",  "rle_symbol = '#'");
    test::roundtrip<ExampleCompressor>("abcccde",     "abcccde",   "rle_symbol = '#'");
}
~~~

Note how options that are not passed will take their default values.

Dynamic options are passed following the simple `name = value` syntax, separated
by `,`. Options for nested algorithms are grouped in brackets (`(` and `)`). The
following example shows this for the `TemplatedExampleCompressor`:

~~~ {.cpp}
test::roundtrip<TemplatedExampleCompressor<ExampleRunEmitter>>
    ("abcccccccde", "abc#6#de",  "encoder(minimum_run = '6', rle_symbol = '#')");
~~~

## The Registry

In order to make a compressor available for the driver application, it needs
to be registered in the driver's [`Registry`](@DX_REGISTRY@).

This is currently possible only by editing the source code file
`src/tudocomp_driver/tudocmp_algorithms.cpp`. Adding the necessary includes and
following lines to the body of the `register_algorithms` function will register
the example compressors:

~~~ {.cpp}
r.register_compressor<ExampleCompressor>();
r.register_compressor<TemplatedExampleCompressor<ExampleRunEmitter>>();
~~~

Note how modular compressors with sub algorithms need every possible combination
registered explicitly.

Once registered, the example compressors are available in the command-line
application and will be listed in the help output. They are also part of the
matrix test when invoking the `check` make target.

## The Comparison Tool

>> *TODO*

