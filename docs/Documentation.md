# Abstract

@URL_DOXYGEN@

The **T**echnical **U**niversity of **DO**rtmund **COMP**ression Framework (*tudocomp*)
is a lossless compression framework with the aim to support and facilitate
the implementation of novel compression algorithms. It already comprises a range
of standard data compression and encoding algorithms. These can be mixed and parameterized with the
following uses in mind:

* Baseline implementations of well-known compression schemes.
* Detailed benchmarking and comparison of compression and encoding algorithms.
* Easy integration of new algorithm implementations.

# Framework Structure

The structure of this framework offers a solid and extensible base for new implementations.
It is designed such that most individual steps are modularized and interchangeable.
This way, the user can mix and match algorithms to find the optimal compression strategy for a given input.
The framework gives this opportunity while creating as little performance overhead as possible.

## Library and driver

The framework consists of two major components: the compression *library*
(*tudocomp*) and the framework utility, called the *driver*. The library
contains the core interfaces and provides implementations of various
compressors; the driver provides the interface for the user in the form of an
executable.

The driver uses a *registry* of compressors, which acts as the link between the
driver and the library. The library is a fully functional standalone library
such that third party applications can make use of the provided compressors.

## Compressor Families

The *compressor families* form the topmost abstraction level in the framework.
Every compression or encoding algorithm belongs to a certain family.

For instance, the compressor family *lzss* (named after *Lempel-Ziv-Storer-Szymanski*)
contains various compressors that factorize
the input resulting in symbols and the produced Lempel-Ziv factors. This output can then be passed to
different encoders specialized for LZSS-type factors to get a binary encoded compressed file.

## Compressors and Modularity

A *compressor*, in terms of this framework, transforms an input sequence and writes the result to an output.
A compressor is the entry point for the utility.

Each compressor family has to implement a decompressor
that can restore the original input losslessly. Apart from that, there are no
strict rules as to *what* kind of transformation of the input occurs. In that sense,
an *encoder* is also a compressor.

Compressors and encoders are implemented in a *modular* way. They are interchangeable and can
be chained (ie the output of one becomes the input of another).

For instance, a factor-based conpressor consists of three main modules:

1. A *factorizer* that produces factors,
2. a *factor encoder* that encodes these factors, and
3. a *raw symbol encoder* that encodes the remaining, unfactorized input.

In this example, the factorizer divides the input into factors that refer to substrings of the input.
The encoders then encode the factors and any unfactorized substrings in an independent manner (e.g.
human readable or bit-optimal).

For each of these tasks, there can be different strategies. For instance, the input can be factorized
using a classic online sliding window approach, but one can also think of using a data structure that works offline and
requires the entire input, such as the suffix array.

Encoders can use myriad representations for the information they encode (e.g. fixed width integers, Huffman codes) which may work better or worse for different types of inputs.

Each of these factorization or encoding strategies can have different sub-strategies
in their own right. The goal of this framework is to modularize compression and encoding
algorithms as much as possible into strategies.

The produced output must contain all information necessary for the respective
decompressor (or decoder) to restore the original input losslessly.

## Runtime Statistics

>> *TODO*: `malloc_count` / `tudostat`

# Usage

## Library

The library comes as a set of `C++` headers, no binary object needs to be built.

>> *TODO*: Not true. `malloc_count` is a binary object and needed for a core feature.

Hence, it suffices to include the respective headers for using a specific compressor implementation.

The [Doxygen documentation](about:blank) provides an overview of
the available compression and encoding implementations.

### Dependencies {#dependencies}

The framework is built using [CMake](https://cmake.org) (2.8 or later).
It is written in `C++11` with GNU extensions and has been tested with the `gcc` compiler family (version 4.9.2 or later)
and `clang` (version 3.5.2 or later).

It has the following external dependencies:

* [SDSL](https://github.com/simongog/sdsl-lite)
  (2.1 or later).
* [Gflags](https://gflags.github.io/gflags) (2.1.2 or later).
* [Google Logging (glog)](https://github.com/google/glog) (0.34 or later).

Additionally, the tests require
[Google Test](https://github.com/google/googletest) (1.7.0 or later).

The CMake build scripts will either find the external dependencies on the build system, or
automatically download and build them from their official repositories in case they cannot be found.

For building the documentation, the following tools are required:

* LATEX (specifically the `pdflatex` component)
* [Doxygen](http://doxygen.org) (1.8 or later).
* [Pandoc](http://pandoc.org) (1.16 or later).
* [Python](https://www.python.org/) (optional, 2.7 or later).
* [pandocfilters (Python module)](https://pypi.python.org/pypi/pandocfilters) (optional, 1.3 or later).

## Framework driver utility

The main executable `tudocomp_driver` is a command line tool that bundles all implemented algorithms.
It provides a fast and easy way to compress and decompress a file with a specified chain of compressors.

It is called the *driver* because it makes available the library functionality for command-line usage.

Every registered compression or encoding algorithm will be listed in the help output of the driver utility
when passing the `--list` command-line argument.

## Building on Windows

On Windows, the framework can be built in a [Cygwin](https://www.cygwin.com/) environment.
`mingw` and Microsoft Visual Studio are *not* supported at this point.

>> *TODO*: Note about `malloc_count`

## License

The framework is published under the [GNU General Public License, Version 3](https://www.gnu.org/licenses/gpl-3.0.en.html).

# GIVE ME A NICE HEADING

This chapter provides a brief introduction of the data flow in the framework.

>> *TODO*: new chapter name, data flow diagram

## The Compressor interface

The [`Compressor`](@URL_DOXYGEN_COMPRESSOR@) class is the foundation for the
compression and decompression cycle of the framework. It defines the two central
operations: `compress` and `decompress`. These are responsible for transforming
an input to an output so that the following (pseudo code) statement is true for
every input:

    decompress(compress(input)) == input

In other words, the transformation must be losslessly reversible by the same
compressor class.

### Magic

In order to identify what compressor has been used to produce a compressed output,
the driver utility can prepend a unique identifier (*magic keyword*) to the output.
This is *not* the responsibility of the compressor.

The identifier is used by the driver when decompressing, to find out what class to use.

## I/O

The framework provides an I/O abstraction in the two classes
[`Input`](@URL_DOXYGEN_INPUT@) and [`Output`](@URL_DOXYGEN_OUTPUT@). Both hide
the actual source or sink of the data (e.g. a file or a place in memory).

### Input

`Input` allows for byte-wise reading from the data source.
It can be used either as a *stream* or as a *view*.

Using a *stream*, characters are read sequentially from the input [^direct-streaming].
The current state of a stream (i.e. the reading position) can be
retained by creating a copy of the stream object - this allows for *rewinding*,
i.e. reading characters again that have already been read.

[^direct-streaming]: Currently, direct streaming from an `std::istream` is not supported. When an `Input`
is constructed from an `istream`, the stream is fully read and buffered in memory. This is an
implementation decision that may change in the future. Note that files, on the other hand,
are not buffered and will always be streamed from disk directly.

A *view* provides random access on the input. This way, the input acts like an
array of characters.

### Output

`Output` provides functionality to stream bytes sequentially to the data sink.

### Bitwise I/O

The [`BitIStream`](@URL_DOXYGEN_BITISTREAM@) and
[`BitOStream`](@URL_DOXYGEN_BITOSTREAM@) classes provide wrappers for bitwise
reading and writing operations. They support reading and writing of single bits,
as well as fixed and variable width integers.

# Tutorial

This chapter provides a guided tour through the implementation of a compressor,
spanning much of the framework's functionality. The following topics will be
discussed:

- Building the framework
- Understanding the framework's file structure
- Writing a simple compressor by implementing the `Compressor`
  interface
- Implementing unit tests
- Adding basic time and memory statistics tracking
- Adding runtime options  select different behavior
- Adding compile time (template) options to your code to select
  different behaviors that should not be selected at runtime due to performance reasons.
- Registering a compressor in the driver registry
- Using the `tudocomp_driver` command line tool with the newly implemented compressor
- Using the `compare_tool` for benchmarking the compressor against other compressors
  for different inputs

You may also refer to an [UML overview](#uml-type-overview) of the framework.

## Building the framework

The tutorial assumes a clean clone of the [*tudocomp* git repository](about:blank).
*tudocomp* is set up as a CMake project[^cmake].

[^cmake]: [CMake](https://cmake.org/) processes the `CMakeLists.txt` files
          throughout the source tree and produces Makefiles for building
          with `make`.

The standard procedure to build is to create a directory named `build` and use
this as CMake's workspace to generate Makefiles. The produced binaries will later
also be placed in the `build` directory tree.

Let us create the build workspace and generate the project Makefiles:

~~~
.../tudocomp> mkdir build
.../tudocomp> cd build
.../tudocomp/build> cmake ..
~~~

When successful, the output ends with `-- Build files have been written to: [...]`.
In case of an error, please make sure the required [dependencies](#dependencies)
are available on the build system.

By default, the project is configured to build in *Debug* mode. While this is
useful for development and debugging, we recommend running benchmarks with the
framework built in *Release* mode. This is achieved by passing the build
configuration to CMake as follows:

~~~
.../build> cmake -DCMAKE_BUILD_TYPE=Release ..
~~~

Debug mode can likewise be configured explicitly:

~~~
.../build> cmake -DCMAKE_BUILD_TYPE=Debug ..
~~~

With the Makefiles generated by CMake, the framework can be built simply
by invoking `make`:

~~~
.../build> make
[... make output ...]
~~~

To ensure everything is working correctly, the `check` target will invoke
the entire unit test range (which can take a while):

~~~
.../build> make check
[... gtest output ...]
All tests were successful!
~~~

>> *TODO*: make target overview

## The framework's file structure

*tudocomp*'s root directory structure follows that of a typical C++ project.
The `include` directory contains the framework's C++ headers, `src` contains
the C++ sources. The unit tests (*Google Test*) are located in `test`. The
remaining directories help structure the project further, but they are not
needed for this tutorial.

Note that *tudocomp* is concepted as a header-only library. While the driver
application and the `malloc_count` module need to be implemented in C++ source
files, all compression and coding algorithm implementations come in the form
of templated classes with inlined functions in the `include` tree.

This is a design decision that allows for the compiler to do heavy code
optimization. Due to the nature of almost everything being templated, this
makes development more convenient as well.

## Implementing a simple compressor

This section presents the steps necessary to implement a simple compressor. The
implementation that is developed here is available in the framework's
repository in the `/include/tudocomp/example/` directory.

### Implementing the Compressor interface

Any compressor needs to implement the [`Compressor`](@URL_DOXYGEN_COMPRESSOR@)
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

The class [`Env`](@URL_DOXYGEN_ENV@) represents the compressor's runtime environment.
It provides access to runtime options as well as the framework's statistics
tracking functionality. A compressor conceptually owns[^cpp11-ownership] its
environment, therefore the constructor takes an rvalue reference to it. The
reference should always be delegated down to the base constructor
(using `std::move`).

[^cpp11-ownership]: This refers to the C++11 ownership semantics, ie. a
`unique_ptr<Env>` is stored internally.

A [`Meta`](@URL_DOXYGEN_META@) object contains information about an algorithm
(e.g. compressors) such as its name and type. This information is used by the
generic algorithm constructor `create_algo`, which will be explained below, as
well as for the registry of the driver utility.

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

The [`tdc`](@URL_DOXYGEN_TDC@) namespace contains most of the core types
required for implementing compressors, including the `Compressor` interface
and the `Env` and `Meta` types.

The `Meta` object returned by `meta()` contains the following information:

* The algorithm type (in this case, a `"compressor"`),
* the algorithm's identifier (for shell compatibility, this should not contain
  any spaces or special characters) and
* a brief description of the algorithm (which would be displayed in the driver
  utility's help output).

### Input and output

In the above example, `compress` and `decompress` do not produce any output.
Generally speaking, (de)compression by means of this framework processes data
by reading from an [`Input`](@URL_DOXYGEN_INPUT@) and writing to an
[`Output`](@URL_DOXYGEN_OUTPUT@).

While the output has to be generated sequentially, the input can be accessed in
two conceptually different ways:

1. As a *stream*, requiring bytes to be read sequentially from the input source
   (the concept of *online* algorithms) or
2. as a *view*, providing random access to the input source as to an array of
   bytes (the concept of *offline* algorithms).

The choice is done by acquiring the respective object from either the
[`as_stream`](@URL_DOXYGEN_INPUT_ASSTREAM@) or the
[`as_view`](@URL_DOXYGEN_INPUT_ASVIEW@) function. The stream object returned by
`as_stream` conforms to the `std::istream` interface and also provides iterator
access. The object returned by `as_view` provides the indexed access `[]`
operator for and the function `size()` to return the amount of bytes available
on the input.

The following code snippet demonstrates using a given input as a view:

~~~ { .cpp }
auto iview = input.as_view(); //retrieve an input view
auto iview2 = iview; // create a shallow copy of the view

// compare the view's content against a certain string
// the CHECK macro is Google Logging's "assert"
CHECK(iview == "foobar");

auto sub_view = iview.substr(1, 5); // create a sub-view for a range within the main view
CHECK(sub_view == "ooba"); // assertion for the sub-view's contents

// iterate over the whole view character-wise
for (size_t i = 0; i < iview.size(); i++) {
    uint8_t c = iview[i];
    // ...
}
~~~

Note that copies and sub-views are shallow, ie. they point to the same memory
location as the original view and thus have the same content.

In contrast, The following code snippet demonstrates using an input as a stream:

~~~ { .cpp }
auto istream = input.as_stream(); // retrieve an input stream
auto istream2 = istream; // create a second stream as a "rewind" position

// read the input character-wise using a C++11 range-based for loop
for(uint8_t c : istream) {
    // ...
}

// read the input character-wise using the std::istream interface
char c;
while(istream2.get(c)) {
    // ...
}
~~~

Note how the framework uses the `uint8_t` (unsigned byte) type to represent
characters. This is contrary to the `std` library, which uses C's `char` type.

Furthermore, note how `istream2` is created as a copy of `istream`. This way,
`istream2` points at the same stream position as `istream` at the time the
copy is created and can be used as a "rewind" point for use independently
of `istream`.

The output has to be generated sequentially and thus only provides a stream
interface. The following code snippet demonstrates this by copying the entire
input to the output:

~~~ { .cpp }
auto istream = input.as_stream(); // retrieve the input stream
auto ostream = output.as_stream(); // retrieve the output stream

// copy the input to the output character by character
for(uint8_t c : istream) {
    ostream << c;
}
~~~

### Example: Run-length encoding

The following example implements the `compress` method so that it yields a
run-length encoding of the input. In run-length encoding, sequences (runs)
of the same character are replaced by one single occurence, followed by the
length of the run.

For example, the input `"abcccccccde"`, which contains a run of six `c`
characters, would be encoded as `"abc%6%de"`, where `%6%` designates that the
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

#include "gtest/gtest.h"

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
from the previous chapter, the run-length encoder:

~~~ { .cpp }
// [/test/example_tests.cpp]

#include "gtest/gtest.h"

#include <tudocomp/example/ExampleCompressor.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

TEST(example, compress) {
    // instantiate the ExampleCompressor
    auto compressor = tdc::create_algo<ExampleCompressor>();

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

*tudocomp* provides the [`create_algo`](@URL_DOXYGEN_CREATEALGO@) function
template that properly instantiates compressors (or more precisely: any class
inheriting from [`Algorithm`](@URL_DOXYGEN_ALGORITHM@)). In this example, the
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
#include "tudocomp_test_util.h"

TEST(example, roundtrip) {
    test::roundtrip<ExampleCompressor>("abcccccccde", "abc%6%de");
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
    test::roundtrip<ExampleCompressor>("abcccccccde", v);
}
~~~

>> *TODO*: We need to consider if the term *roundtrip* should be replaced by
   something more meaningful.

## Runtime Statistics

*tudocomp* provides functionality to measure the running time and the peak
amount of dynamically allocated memory (e.h. via `malloc` or `new`) over the
course of a compression or decompression run.

This functionality is accessible via a compressor's *environment* (represented
by the [`Env`](@URL_DOXYGEN_ENV@) class), which can be retrieved using the
[`env()`](@URL_DOXYGEN_ALGORITH_ENV@) function.

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
[`begin_stat_phase`](@URL_DOXYGEN_ENV_BEGINSTATPHASE@) and
[`end_stat_phase`](@URL_DOXYGEN_ENV_ENDSTATPHASE@) functions like so:

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
[`log_stat`](@URL_DOXYGEN_ENV_LOGSTAT@) method like so:

~~~ { .cpp }
env().log_stat("A statistic", 147);
env().log_stat("Another statistic", 0.5);
~~~

>> *TODO*: Currently, there are only overloads for integer types. Overloads
   for `bool` and `std::string` should be added at least.

Statistic tracking is concluded using the
[`finish_stats`](@URL_DOXYGEN_ENV_FINISHSTATS@) function, which yields a
reference to a [`Stat`](@URL_DOXYGEN_STAT@) object. The JSON can be written to a
stream or retrieved as a string using its [`to_json`](@URL_DOXYGEN_STAT_TOJSON@)
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

>> XXX

## Options

Next we will look at how to add support for runtime flags and options. These are
useful in cases where you want to evaluate your implementation with different
parameters, for example a different dictionary size, a different threshold
value, enabling of optional code paths, etc.

For our example, we will add two options: one to disable the thread sleep
calls, and one to change the escape symbol used by the encoding.

To add these options to your code, you need to first declare them in the
Algorithm's `meta()` method:

~~~ { .cpp }
inline static Meta meta() {
    Meta m("compressor", "example_compressor",
           "This is an example compressor.");
    m.option("debug_sleep").dynamic("false");
    m.option("escape_symbol").dynamic("%");
    return m;
}
~~~

`m.option("...")` declares an option with a specific name,
and the `.dynamic()` indicates that these are runtime options,
as opposed to "templated" ones (We will look at those in the next section).

The argument of `dynamic()` is optional, and provides a default value if
given. In our case, we want to default to no sleep calls, and to the usage of the
`%` symbol.

After defining these options, we can query the value of these
options in the implementation by using the `env().option("...").as_...` family of methods:

~~~ { .cpp }
class ExampleCompressor: public Compressor {
private:
    // The escape symbol gets stored as a field of the class
    const char m_escape_symbol;
public:
    // ...

    inline ExampleCompressor(Env&& env):
        Compressor(std::move(env)),
        // use the first char of the options value
        m_escape_symbol(this->env().option("escape_symbol").as_string()[0])
    {
        // ...
    }

    inline virtual void compress(Input& input, Output& output) override {
        // ...

        bool opt_enable_sleep = env().option("debug_sleep").as_bool();

        auto emit_run = [&]() {
            if (counter > 3) {
                // ...
                ostream << last << m_escape_symbol << counter << m_escape_symbol;
                // ...
            } else {
                // ...
            }
            // ...
        };

        // ...

        if (opt_enable_sleep) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // ...
    }

    // ...
}
~~~

## Option Syntax and Testing

By adding new options to the algorithm we have to add additional tests covering
the new possibilities of how the algorithm may work.
In order to understand how this works,
we briefly look at the command line syntax used for specifying an algorithm and its options.

An algorithm is specified with an id string that concisely
specifies all (sub)algorithms and options it takes. The syntax works
like a function call that supports keyword arguments and default arguments.

As an example, say we have an algorithm `foo` that accepts a string option
`opt1` with default value `'qux'` and a string option `opt2` with default
value `'bar'`, then all of these are valid id strings:

 Id string                            | Full meaning
--------------------------------------|------------
 `foo`                                | `foo(opt1='qux',opt2='bar')`
 `foo()`                              | `foo(opt1='qux',opt2='bar')`
 `foo('x')`                           | `foo(opt1='x',  opt2='bar')`
 `foo('x', 'yz')`                     | `foo(opt1='x',  opt2='yz' )`
 `foo('123', opt2='456')`             | `foo(opt1='123',opt2='456')`
 `foo(opt2 = 'asd', opt1 = 'xkc')  `  | `foo(opt1='xkc',opt2='asd')`
 `foo(opt2 = '...')`                  | `foo(opt1='qux',opt2='...')`

The reason why the syntax is so complex is that the framework supports
a variety of algorithms with many options; thus it needs a concise
notation for specifying a concrete combination of them that works with
default values (regardless of the ordering of the parameters like python, but unlike C++).

Now, if we go back to our example code, both the
`tudocomp::create_algo()` and the `test::roundtrip()` functions
accept an optional string argument that is used for passing options to the algorithm.
This string has to be written in the same syntax as above, but without the enclosing algorithm's name:

~~~ { .cpp }
tudocomp::create_algo<ExampleCompressor>("'/', true");

test::roundtrip<ExampleCompressor>("abcccccccde", "abc-6-de",
                                   "escape_symbol = '-'");
test::roundtrip<ExampleCompressor>("abcccccccde", "abc%6%de",
                                   "debug_sleep = true");
~~~

You can see this in action in the `compress_stats_options` example test.

## Template Options

The runtime options as discussed above are useful,
but they have one aspect that makes them not universally applicable:
Their value is determined at runtime.

This is often fine, but there are scenarios where this can hamper
performance for one simple reason: If the value is only know at
runtime, the compiler can not make optimization decisions
based on its value at compile time.

In the context of the framework, this issue can mostly
be encountered if your algorithm can choose different sub algorithms,
like encoders, or algorithms pre-processing in the input text.

Lets look at an example of what the actual performance problem
can be in that case:

Imagine that your produces some data in a loop.
You want to pass this data on-the-fly to a sub algorithm that processes and encodes it.
Since you can choose different sub algorithms you have a runtime option that selects a specific sub algorithm.
In this case, the code would look somewhat like this:

~~~ { .cpp }
class SubAlgorithmInterface {
    virtual void process_and_encode(...);
}

std::unique_ptr<SubAlgorithmInterface> sub_algo;
std::string sub_algo_name = env().option("...").as_string();

// Select the algorithm to use
if (sub_algo_name == "algo1") {
    sub_algo = std::make_unique<SubAlgo1>();
} else if (sub_algo_name == "algo2") {
    sub_algo = std::make_unique<SubAlgo2>();
}

for(auto byte : input.as_stream()) {
    // ... process(byte)
    data = ...;
    sub_algo.process_and_encode(data);
    // ...
}
~~~

The bottleneck in this code is the `process_and_encode()` call inside the
loop.
Because the compiler does not know which of the algorithms will be
selected at runtime, it needs to emit native code that does a virtual call
(that is, it loads the function pointer for the method from a
[vtable](https://en.wikipedia.org/wiki/Virtual_method_table) and calls it).
Besides causing overhead,
the compiler is not able to "merge" the native code of the function body
with the native code of the loop for a more efficient and faster compiled
program.

>> *TODO*: Why do you emphasize on "native" code?

In order to enable these optimizations, the compiler needs to know
at compile time what function or method of what object will be called.
For standalone functions, it knows this if the function is defined in the
same
`*.cpp` file, or exists with a `inline` annotation in a header
(hence the header-only design of the library).

>> *TODO*: why is inline necessary? This is only a compiler hint

For the above example, the recommended way is to turn
the sub algorithm into a generic template parameter of the class:

~~~ { .cpp }
template<class T>
class MyCompressor {
    // ...
    void compress(...) {
        // ...
        T sub_algo = T();
        for(auto byte : input.as_stream()) {
            // ... process(byte)
            data = ...;
            sub_algo.process_and_encode(data);
            // ...
        }
    }
}
~~~

This transforms the selection of the sub algorithm
from a runtime lookup in a compile time decision between
`MyCompressor<SubAlgo1>` and `MyCompressor<SubAlgo2>`.

By compiling code using one of those types, it will generate
native code
for the `SubAlgo1` case, or native code for the `SubAlgo2` case,
in either case having full knowledge of the code inside the loop body, which allows
further optimizations.

Now let us head back to our example compressor.
Here, we want to make the encoding of the repeated characters exchangeable.
We will do this by adding a sub algorithm that will encode this kind of repetitions.

Because this modification changes the existing example code more seriously,
we make a copy of the `ExampleCompressor` class from the
`ExampleCompressor.hpp` file.
We rename the copied class to `TemplatedExampleCompressor`.

First, we make the class generic and give it a private
member that stores an instance of the chosen encoder:

~~~ { .cpp }
template<typename T>
class TemplatedExampleCompressor: public Compressor {
private:
    // We want to keep an instance of the encoder
    // alive for the same duration as the compressor itself, so we
    // keep it as a field
    T m_encoder;
~~~

The sub algorithm does _not have_ to be a field of the compressor.
Depending on the design of the algorithm, the class could also
be instantiated in the `compress()` or `decompress()` methods.

As a second step, we declare the sub algorithm as an "templated" option in our meta method,
since we want to make the choice of the encoder visible as an option.

~~~ { .cpp }
public:
    inline static Meta meta() {
        Meta m("compressor", "templated_example_compressor",
               "This is a templated example compressor.");
        m.option("encoder").templated<T>();
        m.option("debug_sleep").dynamic("false");
        return m;
    }
~~~

We have removed the `escape_symbol` option, because it is now specified by the used encoder.

Exactly as with `dynamic()`, we have the option of giving a default
"value" for the `templated()` options. Specifically,
the syntax looks like `.templated<T, ActualType>()`.
Note that this will not automatically make this the default for the template
parameter of the class itself, just for the registry and command line tool.

>> *TODO*: Do you mean that it does not mean that this does not cause a default template parameter like <T = ActualType>

Next, we initialize the encoder in the constructor of the compressor.
The actual API of a sub algorithm can be arbitrary, with almost no restrictions on the amount of methods
and the life cycle of the instances themselves.

However, if you want to use the templated parameter as an proper algorithm option
then you should make sure that your sub algorithms inherit from `tudocomp::Algorithm`,
and have a constructor that takes an `Env&&` as the first argument that has to be delegated
to its base class constructor.
By following these steps, we can initialize it with a new `Env` instance created by
`env().env_for_option("<option_name>")`. This ensures that the options for a
sub algorithm will be delegated to its actual constructor:

~~~ { .cpp }
    inline TemplatedExampleCompressor(Env&& env):
        Compressor(std::move(env)),
        // Initialize it with an `Env` for all its options
        m_encoder(this->env().env_for_option("encoder")) {}
~~~

We finalize the changes in the compressor class by calling the encoder in the actual compression routine.
For this purpose, we define four methods that a suitable "ExampleEncoder" needs to provide:

- `size_t threshold()` returns an integer for the minimum repeat count
that should be replaced for the encoder. For example, if you replace `"aaa..."`
with `a%N%` then that will reduce the size only for repeats longer than three.
- `void encode_repeat(char last, size_t repeat, io::OutputStream& ostream)`  encodes a repetition of length repeat consisting of the same character char.
- `bool is_start_of_encoding(const io::InputView& iview, size_t i)` identifies the start of a character repetition in the decoder.
- `size_t decode(const io::InputView& iview, size_t& i)` decodes a character repetition and returns the length of this repetition (while advancing the input cursor at position i in the process).

>> *TODO*: cursor undef

(This interface is an entirely arbitrary example, every algorithm can define its own design patterns.)

Integrated in the example code, it looks like this:

~~~ { .cpp }
    inline virtual void compress(Input& input, Output& output) override {
        auto istream = input.as_stream();
        auto ostream = output.as_stream();

        char last;
        char current;
        size_t counter = 0;

        size_t stat_max_repeat_counter = 0;
        size_t stat_count_repeat_segments = 0;
        bool opt_enable_sleep = env().option("debug_sleep").as_bool();

        auto emit_run = [&]() {
            if (counter > m_encoder.threshold()) {
                // Delegate encoding to encoder
                m_encoder.encode_repeat(last, counter, ostream);

                stat_count_repeat_segments++;
            } else {
                // Else output chars as normal
                for (size_t i = 0; i <= counter; i++) {
                    ostream << last;
                }
            }
            stat_max_repeat_counter = std::max(stat_max_repeat_counter,
                                               counter);
        };

        env().begin_stat_phase("run length encoding");

        istream.get(last);
        while(istream.get(current)) {
            if (current == last) {
                counter++;
            } else {
                // Emit run length encoding here
                emit_run();
                // Then continue with the next character
                last = current;
                counter = 0;
            }
        }
        // Don't forget trailing chars
        emit_run();

        env().log_stat("max repeat", stat_max_repeat_counter);
        env().log_stat("count repeat segments", stat_count_repeat_segments);

        if (opt_enable_sleep) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // FIXME, adding a fake memory allocation
            std::vector<uint8_t> v(100 * 1024, 42); // 100 KiB of the byte 42
        }
        env().end_stat_phase();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        // Use the input as a memory view here, just to have both variants used
        auto iview = input.as_view();
        auto ostream = output.as_stream();

        char last = '?';

        for (size_t i = 0; i < iview.size(); i++) {
            if (m_encoder.is_start_of_encoding(iview, i)) {
                size_t counter = m_encoder.decode(iview, i);
                for (size_t x = 0; x < counter; x++) {
                    ostream << last;
                }
            } else {
                ostream << iview[i];
                last = iview[i];
            }
        }
    }
};

~~~

Finally, let us look at the implementation for the actual encoder classes.
For our example, we will define two encoders: one doing the existing
human readable encoding of repeats (`"aaaa" -> "a%3%"`), and another one
computing a binary oriented output, where (instead of a readable number) it
encodes the length as a byte directly (`"aaaa" -> [97, 255, 3]`, with `255`
being the escape symbol).

Again, for the sake of simplicity, we don't address some of the hidden bugs
like `'%'` or `'\xff'` appearing in the input text, or the case that there is
a character repetition longer than what fits in a byte.

~~~ { .cpp }
class ExampleDebugCoder: public Algorithm {
private:
    const char m_escape_symbol;
public:
    inline ExampleDebugCoder(Env&& env):
        Algorithm(std::move(env)),
        m_escape_symbol(this->env().option("escape_symbol").as_string()[0]) {}

    inline static Meta meta() {
        Meta m("example_coder", "debug",
               "This is an example debug coder, encoding human readable.");
        m.option("escape_symbol").dynamic("%");
        return m;
    }

    inline void encode_repeat(char last, size_t repeat, io::OutputStream& ostream) {
        ostream << last << m_escape_symbol << repeat << m_escape_symbol;
    }

    inline bool is_start_of_encoding(const io::InputView& iview, size_t i) {
        return iview[i] == m_escape_symbol;
    }

    inline size_t decode(const io::InputView& iview, size_t& i) {
        size_t counter = 0;
        for (i++; i < iview.size(); i++) {
            if (iview[i] == m_escape_symbol) {
                break;
            } else {
                counter *= 10;
                counter += (iview[i] - '0');
            }
        }
        return counter;
    }

    inline size_t threshold() { return 3; }
};

class ExampleBitCoder: public Algorithm {
private:
    const char m_escape_symbol;
public:
    inline ExampleBitCoder(Env&& env):
        Algorithm(std::move(env)),
        m_escape_symbol(this->env().option("escape_byte").as_integer()) {}

    inline static Meta meta() {
        Meta m("example_coder", "bit",
               "This is an example bit coder, encoding as a binary integer.");
        m.option("escape_byte").dynamic("255");
        return m;
    }

    inline void encode_repeat(char last, size_t repeat, io::OutputStream& ostream) {
        ostream << last << m_escape_symbol << char(uint8_t(repeat));
    }

    inline bool is_start_of_encoding(const io::InputView& iview, size_t i) {
        return iview[i] == uint8_t(m_escape_symbol);
    }

    inline size_t decode(const io::InputView& iview, size_t& i) {
        i++;
        size_t counter = uint8_t(iview[i]);
        return counter;
    }

    inline size_t threshold() { return 2; }
};
~~~

### Tests

As a subsequent step we want to verify that the templated version of the compressor works as
intended. To this end, we use the same testing as before:

~~~ { .cpp }
using tudocomp::TemplatedExampleCompressor;
using tudocomp::ExampleDebugCoder;
using tudocomp::ExampleBitCoder;

TEST(example, templated_debug) {
    test::roundtrip<TemplatedExampleCompressor<ExampleDebugCoder>>(
        "abbbbcccccccde", "abbbbc%6%de");

    test::roundtrip<TemplatedExampleCompressor<ExampleDebugCoder>>(
        "abbbbcccccccde", "abbbbc%6%de", "debug()");

    test::roundtrip<TemplatedExampleCompressor<ExampleDebugCoder>>(
        "abbbbcccccccde", "abbbbc-6-de", "debug(escape_symbol = '-')");
}

TEST(example, templated_bit) {
    test::roundtrip<TemplatedExampleCompressor<ExampleBitCoder>>(
        "abbbbcccccccde",
        std::vector<uint8_t> { 'a', 'b', 0xff, 3, 'c', 0xff, 6, 'd', 'e' });

    test::roundtrip<TemplatedExampleCompressor<ExampleBitCoder>>(
        "abbbbcccccccde",
        std::vector<uint8_t> { 'a', 'b', 0xff, 3, 'c', 0xff, 6, 'd', 'e' },
        "bit()");

    test::roundtrip<TemplatedExampleCompressor<ExampleBitCoder>>(
        "abbbbcccccccde",
        std::vector<uint8_t> { 'a', 'b', 0, 3, 'c', 0, 6, 'd', 'e' },
        "bit(escape_byte = '0')");
}
~~~

The template options will set the corresponding parameters
in an option string to a fixed value.
If you explicitly set some options
you either have to skip the option parameters belonging to the template parameter,
or you have to give it the same value as indicated by it; otherwise you will get an error:

>> *TODO*: not really clear

~~~ { .cpp }
// ok
tudocomp::create_algo<TemplatedExampleCompressor<ExampleDebugCoder>>("debug(escape_symbol = '-')");
// error
tudocomp::create_algo<TemplatedExampleCompressor<ExampleDebugCoder>>("bit(escape_byte = '123')");
~~~

## The Registry

Once a compressor implementation exists, and has a few unit tests that
verify the basic operation, it can be registered in the global
algorithm registry in order to be usable from the command line tool.
Adding a compressor to the registry will also result in getting picked by a few automatic unit tests
that run all algorithms with a number of small test cases.

Doing this is easy: In `src/tudocomp_driver/tudocmp_algorithms.cpp`,
add includes for your compressor headers, and then add new `r.compressor<T>()`
lines for all compressor classes in the function body there.

>> *TODO*: where is the function body?

For the templated version, you need to register the class with
all possible combinations of type parameters it can accept, in order to generate native code instance for all of them.

For our running example, we add these three lines:

~~~ { .cpp }
r.compressor<ExampleCompressor>()
r.compressor<TemplatedExampleCompressor<ExampleDebugCoder>>();
r.compressor<TemplatedExampleCompressor<ExampleBitCoder>>();
~~~

If we now run `make check`
(or directly `make tudocomp_driver_algorithm_matrix_tests`), the new
compressors should be picked by the "tudocomp_driver_algorithm_matrix_tests"
target, which will run the previously mentioned tests on them:

~~~
...
example_compressor_0.txt -> example_compressor_0.tdc -> example_compressor_0.decomp.txt ... OK
example_compressor_1.txt -> example_compressor_1.tdc -> example_compressor_1.decomp.txt ... OK
example_compressor_2.txt -> example_compressor_2.tdc -> example_compressor_2.decomp.txt ... OK
example_compressor_3.txt -> example_compressor_3.tdc -> example_compressor_3.decomp.txt ... OK
...
~~~

## The Driver

After integrating a compressor into the registry, we can start using it
with the command line tool.

The "Driver" (named this way because it serves as an user interface that
drives the underlying tudocomp library) can be compiled with `make tudocomp_driver`.
The executable of the driver is stored in `build/src/tudocomp_driver/tudocomp_driver` (after compilation).

As a first step after building it, we can verify that the new compressors
exists by listing all known algorithms:

~~~
.../build> ./src/tudocomp_driver/tudocomp_driver --list
This build supports the following algorithms:

  [Compression algorithms]
  -------------------------------------------------------------------------------------------------------------------------------
  | ...                                                            | ...                                                        |
  -------------------------------------------------------------------------------------------------------------------------------
  | example_compressor(escape_symbol = "%", debug_sleep = "false") | This is an example compressor.                             |
  |   where `escape_symbol` is one of [string],                    |                                                            |
  |         `debug_sleep` is one of [string]                       |                                                            |
  -------------------------------------------------------------------------------------------------------------------------------
  | templated_example_compressor(encoder, debug_sleep = "false")   | This is a templated example compressor.                    |
  |   where `encoder` is one of [example_coder],                   |                                                            |
  |         `debug_sleep` is one of [string]                       |                                                            |
  -------------------------------------------------------------------------------------------------------------------------------

  [Argument types]
    [example_coder]
    -----------------------------------------------------------------------------------------------------------
    | debug(escape_symbol = "%")                 | This is an example debug coder, encoding human readable.   |
    |   where `escape_symbol` is one of [string] |                                                            |
    -----------------------------------------------------------------------------------------------------------
    | bit(escape_byte = "255")                   | This is an example bit coder, encoding as a binary integer.|
    |   where `escape_byte` is one of [string]   |                                                            |
    -----------------------------------------------------------------------------------------------------------

  ...
~~~

Next we will compress something with the tool. Since this is
a quick test, we use an already existing file in the build
directory, the `CMakeCache.txt`:

~~~
.../build> ./src/tudocomp_driver/tudocomp_driver -algorithm "example_compressor" CMakeCache.txt -output ./cache.tdc -stats
~~~

This will create a compressed file `cache.tdc` in the current directory, and
print some statistics to the command line, including the JSON data that
can be pasted into the previously mentioned web visualizer.

> *Note*: The tool will prevent accidental overwrites if a file with the
output filename already exists. To bypass this security mechanism, and hence allow overwriting,
the `-force` option has to be used.

The stat output will likely show a very small compression effect, if any at
all. But the file format contains a few sections decorated with
comments containing repeats of the same character.
If you look at the contents of `cache.tdc`, say with `cat cache.tdc | less`, you should be able to see some
encoding sequences in the file.

As a point of reference, on the authors' machine it only resulted in a 1% reduction, with 4 actual run length replacements.

For a last check, we can see if the decompression works as well:

~~~
.../build> ./src/tudocomp_driver/tudocomp_driver -decompress cache.tdc -output cache.orig.txt
.../build> diff -s CMakeCache.txt cache.orig.txt
Files CMakeCache.txt and cache.orig.txt are identical
~~~

... which seems to be the case.

## The Compare Tool

Last but not least, the framework provides a tool for
automatically running different command line tools on different input text
files.

This "compare_tool" can be used to compare the effects of a compression
algorithm on different classes of input text, or the effects of
different algorithms on the same input files. It can be build with `make compare_tool`.

It's main purpose is to compare the runtime and compression ratio of compressors combined with different options and input files.
However, it does not integrate into the framework's own statistics tracking.

> *Note*: There is also a separate, experimental support for measuring the memory
footprint with the Valgrind tool massif, but this does not work well with
external compression programs. Further, it seems redundant in the light of the memory statistic tracking methods.

The compare tool works as follows:
It takes a config file as a parameter and a profile name.
The config file contains a number of comparison profiles, where each profile has a name.
The compare tool executes the profile in the config file whose name matches with the name passed as the parameter.
While executing the profile, the compare tool prints the results to stdout.

A profile consists of a few options, and two lists. The one list contains the commands
to run, and the other contains the input files. The compare tool
builds the Cartesian product of both lists, and executes each combination
in turn, noting time and compressed size. In the following, we call such a combination a benchmark run.

The config file obeys the [TOML](https://github.com/toml-lang/toml) syntax,
and expects one or more profiles defined like this:

~~~ { .toml }
[PROFILE_NAME]
compare_commands = true # or false
inputs = [
    "FILE1",
    "FILE2",
    # ...
]
commands = [
    ["COMMAND1 --WITH_OPTIONS", "--ADDITIONAL_OPTIONS"],
    ["COMMAND2 --WITH_OPTIONS", "--ADDITIONAL_OPTIONS"],
    # ...
]

[PROFILE2_NAME]
# ...
~~~

`inputs` is the list of input files, and `commands` is the list
of command line commands to run. The commands are split into two
strings that will be merged by the tool. The reason for this split
is that the first part of the command line acts as a label, while
the second part contains some boilerplate arguments (e.g., correctly directing the
input and the output, while being irrelevant for identifying the tool and
algorithm itself).

You can use in the command strings the two special variables `$IN` and `$OUT`
that are substituted by the compare tool with file paths to the input and output filename, respectively.

The `compare_commands` flag decides how the benchmark runs are grouped.
If it is set to `true`, it will run each command in turn on the same input
file; if it is set to `false`, it will run the same command with all input
files:

 `compare_commands = true` | `compare_commands = false`
---------------------------|----------------------------
 `FILE1` run on `COMMAND1`, `COMMAND2` | `COMMAND1` run with `FILE1`, `FILE2`
 `FILE2` run on `COMMAND1`, `COMMAND2` | `COMMAND2` run with `FILE1`, `FILE2`
 ...                                   | ...

### Datasets

Tudocomp provides a target `datasets` that will download a selection
of files, mostly from the [Pizza&Chili Corpus](http://pizzachili.dcc.uchile.cl/texts.html).
At the point of writing this document, the datasets take 9.4 GiB of space.
Make sure you have enough free storage before doing this!

~~~
.../build> make datasets
~~~

> *Note* that the make target can be aborted at any time and will resume
> with what it had already downloaded the next time it gets invoked.

The downloaded files can be found in `/etc/datasets/download`.

This example will use some of those texts.

### Compare Workspace

At this point we already can build the compare tool
and invoke it with a config file.
But this is a bit cumbersome, because:

- The input files and the compare tool can have complex file paths.
- The compare files dumps log files and compression artifacts to the current
  working directory, so it should run in its own directory.

  >> *TODO*: what are compare files dumps log files?

- Changes to the tudocomp codebase will not automatically be recognized
  by the tool.

To address these (arguably minor) concerns the build system offers the target
`compare_workspace`. It will create the directory `/build/compare_workspace`,
and fill it with a short shell script `run.sh` that:

- Invokes the make targets `tudocomp_driver`, `compare_tool` and
  `compare_workspace` to ensure that the built code is up to date.
- Locally appends the binary directory of Tudocomp to`$PATH` such that it does not have to be
  installed or be invoked with the relative/full path.

>> *TODO*: what is "it"?

- Defines an environment variable `$DATASETS` pointing
  to the dataset directory.
- Delegates to the `compare_tool` with the environment
  defined above.

>> *TODO*: what will be delegated? The environment variables? Parameters like below?

This set-up enables us to quickly create a test environment for different algorithms:

~~~
.../build> make compare_workspace
.../build> cd compare_workspace
.../compare_workspace> ./run.sh config.toml example
...
~~~

### An Example Config File

As the last step of this tutorial, we will create a compare config file in the workspace directory for our running example.

Our goal is to compare the three compressor variants we had
defined with a few different input files: English text,
source code, DNA sequences, etc. We use the datasets we downloaded in the previous step.

We create a new text file `/build/compare_workspace/example_config.toml`, and
give it the following content:

~~~ { .toml }
[example]
compare_commands = true
inputs = [
    "$DATASETS/download/real/english.50MB",
    "$DATASETS/download/real/sources.50MB",
    "$DATASETS/download/real/dblp.xml.50MB",
    "$DATASETS/download/real/proteins.50MB",
    "$DATASETS/download/real/dna.50MB",
]
commands = [
    ["tudocomp_driver -algorithm 'example_compressor'",                  "-raw $IN -output $OUT"],
    ["tudocomp_driver -algorithm 'templated_example_compressor(bit)'",   "-raw $IN -output $OUT"],
    ["tudocomp_driver -algorithm 'templated_example_compressor(debug)'", "-raw $IN -output $OUT"],
]
~~~

This content defines a config file with the profile `example`.
The profile `example` runs the algorithms `example_compressor`, `templated_example_compressor(bit)`
and `templated_example_compressor(debug)` on five input texts.
The benchmark runs are grouped by the input files since we want to compare the differences between the algorithms on the same text.

By invoking the run script with this config, we get an output like this:

~~~
.../compare_workspace> ./run.sh example_config.toml example
[ 13%] Built target malloc_count
[ 47%] Built target sdsl_external
[ 82%] Built target gflags_external
[ 91%] Built target tudocomp_algorithms
[100%] Built target tudocomp_driver
[100%] Built target rust_external
[100%] Built target compare_tool
[100%] Built target compare_workspace

Profile: example

file | $DATASETS/download/real/english.50MB                             |   time    |     size     |  comp. size  |  ratio   |     mem
-------------------------------------------------------------------------------------------------------------------------------------------
cmd  | tudocomp_driver -algorithm 'example_compressor'                  |~  1.681 s |    50.00 MiB |    49.85 MiB |  99.71 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(bit)'   |~  1.681 s |    50.00 MiB |    49.81 MiB |  99.63 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(debug)' |~  1.680 s |    50.00 MiB |    49.85 MiB |  99.71 % |      -
-------------------------------------------------------------------------------------------------------------------------------------------
file | $DATASETS/download/real/sources.50MB                             |   time    |     size     |  comp. size  |  ratio   |     mem
-------------------------------------------------------------------------------------------------------------------------------------------
cmd  | tudocomp_driver -algorithm 'example_compressor'                  |~  1.612 s |    50.00 MiB |    48.49 MiB |  96.97 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(bit)'   |~  1.605 s |    50.00 MiB |    48.10 MiB |  96.20 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(debug)' |~  1.679 s |    50.00 MiB |    48.49 MiB |  96.97 % |      -
-------------------------------------------------------------------------------------------------------------------------------------------
file | $DATASETS/download/real/dblp.xml.50MB                            |   time    |     size     |  comp. size  |  ratio   |     mem
-------------------------------------------------------------------------------------------------------------------------------------------
cmd  | tudocomp_driver -algorithm 'example_compressor'                  |~  1.641 s |    50.00 MiB |    50.00 MiB | 100.00 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(bit)'   |~  1.686 s |    50.00 MiB |    50.00 MiB |  99.99 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(debug)' |~  1.772 s |    50.00 MiB |    50.00 MiB | 100.00 % |      -
-------------------------------------------------------------------------------------------------------------------------------------------
file | $DATASETS/download/real/proteins.50MB                            |   time    |     size     |  comp. size  |  ratio   |     mem
-------------------------------------------------------------------------------------------------------------------------------------------
cmd  | tudocomp_driver -algorithm 'example_compressor'                  |~  1.717 s |    50.00 MiB |    49.96 MiB |  99.93 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(bit)'   |~  1.637 s |    50.00 MiB |    49.92 MiB |  99.83 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(debug)' |~  1.724 s |    50.00 MiB |    49.96 MiB |  99.93 % |      -
-------------------------------------------------------------------------------------------------------------------------------------------
file | $DATASETS/download/real/dna.50MB                                 |   time    |     size     |  comp. size  |  ratio   |     mem
-------------------------------------------------------------------------------------------------------------------------------------------
cmd  | tudocomp_driver -algorithm 'example_compressor'                  |~  1.997 s |    50.00 MiB |    49.31 MiB |  98.62 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(bit)'   |~  1.794 s |    50.00 MiB |    48.21 MiB |  96.41 % |      -
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(debug)' |~  1.852 s |    50.00 MiB |    49.31 MiB |  98.62 % |      -
-------------------------------------------------------------------------------------------------------------------------------------------

~~~

We see that the algorithms are not very effective.
As expected, there is no visible difference between the
first compressor, and the second one with the debug encoder.

We can also toggle the `compare_commands` option to group
by the algorithms instead; this allows us to compare how an algorithms performs on different inputs:

~~~
.../compare_workspace> ./run.sh example_config.toml example
[ 34%] Built target sdsl_external
[ 60%] Built target malloc_count
[ 82%] Built target gflags_external
[ 91%] Built target tudocomp_algorithms
[100%] Built target tudocomp_driver
[100%] Built target rust_external
[100%] Built target compare_tool
[100%] Built target compare_workspace

Profile: example

cmd  | tudocomp_driver -algorithm 'example_compressor'                  |   time    |     size     |  comp. size  |  ratio   |     mem
-------------------------------------------------------------------------------------------------------------------------------------------
file | $DATASETS/download/real/english.50MB                             |~  1.829 s |    50.00 MiB |    49.85 MiB |  99.71 % |      -
file | $DATASETS/download/real/sources.50MB                             |~  1.697 s |    50.00 MiB |    48.49 MiB |  96.97 % |      -
file | $DATASETS/download/real/dblp.xml.50MB                            |~  1.787 s |    50.00 MiB |    50.00 MiB | 100.00 % |      -
file | $DATASETS/download/real/proteins.50MB                            |~  1.643 s |    50.00 MiB |    49.96 MiB |  99.93 % |      -
file | $DATASETS/download/real/dna.50MB                                 |~  1.874 s |    50.00 MiB |    49.31 MiB |  98.62 % |      -
-------------------------------------------------------------------------------------------------------------------------------------------
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(bit)'   |   time    |     size     |  comp. size  |  ratio   |     mem
-------------------------------------------------------------------------------------------------------------------------------------------
file | $DATASETS/download/real/english.50MB                             |~  1.678 s |    50.00 MiB |    49.81 MiB |  99.63 % |      -
file | $DATASETS/download/real/sources.50MB                             |~  1.664 s |    50.00 MiB |    48.10 MiB |  96.20 % |      -
file | $DATASETS/download/real/dblp.xml.50MB                            |~  1.630 s |    50.00 MiB |    50.00 MiB |  99.99 % |      -
file | $DATASETS/download/real/proteins.50MB                            |~  1.672 s |    50.00 MiB |    49.92 MiB |  99.83 % |      -
file | $DATASETS/download/real/dna.50MB                                 |~  1.800 s |    50.00 MiB |    48.21 MiB |  96.41 % |      -
-------------------------------------------------------------------------------------------------------------------------------------------
cmd  | tudocomp_driver -algorithm 'templated_example_compressor(debug)' |   time    |     size     |  comp. size  |  ratio   |     mem
-------------------------------------------------------------------------------------------------------------------------------------------
file | $DATASETS/download/real/english.50MB                             |~  1.650 s |    50.00 MiB |    49.85 MiB |  99.71 % |      -
file | $DATASETS/download/real/sources.50MB                             |~  1.746 s |    50.00 MiB |    48.49 MiB |  96.97 % |      -
file | $DATASETS/download/real/dblp.xml.50MB                            |~  1.638 s |    50.00 MiB |    50.00 MiB | 100.00 % |      -
file | $DATASETS/download/real/proteins.50MB                            |~  1.755 s |    50.00 MiB |    49.96 MiB |  99.93 % |      -
file | $DATASETS/download/real/dna.50MB                                 |~  1.838 s |    50.00 MiB |    49.31 MiB |  98.62 % |      -
-------------------------------------------------------------------------------------------------------------------------------------------

~~~

Regarding the compression ratio, the source code file seems to compress best, presumably due
the repetition of indentation characters like tabs and spaces.

# UML Type Overview

![The Tudocomp API](media/project_setup.png)

# Credits

*tudocomp* was created with the help of ...

| Name | Roles |
| ---- | ----- |
| Dinklage, Patrick | core development, research, documentation |
| Köppl, Dominik    | supervision, advice, research |
| Löbel, Marvin     | core development, documentation |
