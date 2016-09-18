# Abstract

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

>TODO: `malloc_count` / `tudostat`

# Usage

## Library

The library comes as a set of `C++` headers, no binary object needs to be built.
>TODO: Not true. `malloc_count` is a binary object and needed for a core feature.

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
* [`gflags`](https://gflags.github.io/gflags) (2.1.2 or later).
* [`glog`](https://github.com/google/glog) (0.34 or later).

Additionally, the tests require
[Google Test](https://github.com/google/googletest) (1.7.0 or later).

The CMake build scripts will either find the external dependencies on the build system, or
automatically download and build them from their official repositories in case they cannot be found.

For building the documentation, the following tools are required:

* LATEX (specifically the `pdflatex` component)
* [Pandoc](http://pandoc.org)
* [Doxygen](http://doxygen.org)

## Framework driver utility

The main executable `tudocomp_driver` is a command line tool that bundles all implemented algorithms.
It provides a fast and easy way to compress and decompress a file with a specified chain of compressors.

It is called the *driver* because it makes available the library functionality for command-line usage.

Every registered compression or encoding algorithm will be listed in the help output of the driver utility
when passing the `--list` command-line argument.

## Building on Windows

On Windows, the framework can be built in a [Cygwin](https://www.cygwin.com/) environment.
`mingw` and Microsoft Visual Studio are *not* supported at this point.

>TODO: Note about `malloc_count`

## License

The framework is published under the [GNU General Public License, Version 3](https://www.gnu.org/licenses/gpl-3.0.en.html).

# GIVE ME A NICE HEADING

This chapter provides a brief introduction of the data flow in the framework.

>TODO: new chapter name, data flow diagram

## The Compressor interface

The [`Compressor`](about:blank) class is the foundation for the compression and
decompression cycle of the framework. It defines the two central operations:
`compress` and `decompress`. These are responsible for transforming an input to
an output so that the following (pseudo code) statement is true for every input:

    decompress(compress(input)) == input

In other words, the transformation must be losslessly reversible by the same
compressor class.

### Magic

In order to identify what compressor has been used to produce a compressed output,
the driver utility can prepend a unique identifier (*magic keyword*) to the output.
This is *not* the responsibility of the compressor.

The identifier is used by the driver when decompressing, to find out what class to use.

## I/O

The framework provides an I/O abstraction in the two classes [`Input`](about:blank) and
[`Output`](about:blank). Both hide the actual source or sink of
the data (e.g. a file or a place in memory).

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

The [`BitIStream`](about:blank) and [`BitOStream`](about:blank) classes provide
wrappers for bitwise reading and writing operations.
They support reading and writing of single bits, as well as fixed and variable
width integers.

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

## Building the Framework

The tutorial assumes a clean clone of the [tudocomp git repository](about:blank).
tudocomp is set up as a CMake project[^cmake].

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
In case of an error, please make sure the required [dependencies](#dependencies) are
available on the build system.

>LESEZEICHEN

The command `cmake ..` configures the build in debugging mode per default;
this means that the compiled C++ code will not be optimized. Instead, it will
be enhanced with debugging information, convenient for debuggers like `gdb`.

The debugging mode is usually wanted for development,
but is unhelpful for comparing the performance.
If you want to compile your code optimized you can explicitly tell `cmake` to configure the project in "Release" mode:

~~~
.../build> cmake -DCMAKE_BUILD_TYPE=Release ..
~~~

There is also the option to explicitly configure the project in "Debug" mode by writing this:

~~~
.../build> cmake -DCMAKE_BUILD_TYPE=Debug ..
~~~

The next step is to build the library.
This could take a while at the first time,
since it might have to download and compile a few external libraries on which the project depends:

~~~
.../build> make
[...] lots of output [...]
~~~

Finally, we can run the test suite and generate the docs:

~~~
.../build> make check
[...]
All tests were successful!
.../build> make docs
[...]
~~~

## Where and How to Add Files

After you have verified in the previous step that the basic
framework compiled correctly, its time to look
at how the source files are organized.

The source tree contains a few different things:
The framework library, the command line driver, the algorithm registry used by the former, the test suite and the compare tool.

The `.cpp` files for these parts live in their own directories
in `/src`, and the header files live in their own directories
in `/include`.

The library of the framework is organized as a header-only C++ library,
which means that all code lives in header files, and none in `.cpp` source files.

This is done for two reasons:

- Performance: If the code lives in headers, then it can get
  optimized better since the compiler can always see all of it, and
  thus can do optimizations it could not do if parts of the code would
  only be linked together from another compiled `.cpp` file.
- Convenience: A lot of the code is templated, and thus would live in
  in a header anyway. But we also moved the code that does not necessarily
  _have_ to be put in a header file. By doing so, we get rid of the typical C++ way
  of splitting up all the code between header and `.cpp` files.

> Note: This is not a hard design decision, and might change again
> depending on how the framework develops.

If there is some uncertainty about how header-only C++ code works,
note that the basic idea is
that any function or other definition not defined as a template
gets annotated with `inline`, which allows putting its
implementation directly in the header.
>TODO: I do not understand this paragraph. Why is it useful to know inline if you do not know header-only?

We now start with adding a new compressor to the framework.
Our aim is to create a new header file somewhere in `/include/tudocomp/` or in a new subdirectory in it.

For this tutorial, we will pick `/include/tudocomp/example/`.

> Note: All example code introduced in this Tutorial exists
> as actual example files in the source code of the framework.

## A Simple Compressor

### The interface

Adding a new compressor starts with
implementing the basic interface for a Tudocomp compressor.

Since the framework is a template library, this involves
having the right members and properties in your class, rather than
inheriting from an abstract interface.
>TODO: this does not make sense since you do inherit (see below)

Specifically, to implement a Tudocomp compressor
the following interface needs to hold for the class:

- The class needs to publicly inherit from the `tudocomp::Compressor`
  interface and implement its virtual methods.
- The class needs a constructor with the signature `(tudocomp::Env&&)` that
  delegates the `Env` variable to the `Compressor` constructor.
- The class needs to have a static member function `meta()`
  that returns an instance of the `tudocomp::Meta` type.

`Env` is a helper class for accessing the environment around the algorithm; it
gives you access to runtime flags, and the
statistic tracking framework. Conceptually, a compressor owns its environment,
which is why the constructor takes ownership through the rvalue reference. It
should always be delegated to the base class constructor, since that will
take care of storing it. The base class provides access to the `Env` instance with the `.env()` getter method.

`Meta` contains metadata that is important for the algorithm to identify itself,
and to declare what additional options it might take. It is mainly used by the
registry to tell the algorithms apart at runtime. Apart from that it is also needed for
the correct construction of your class since the environment will have
to be initialized in an algorithm-specific way by the framework.
>TODO: what does algorithm-specific way mean?

For more details on those types see the API docs, or the headers
`tudocomp/Compressor.hpp`, `tudocomp/Env.hpp` and `tudocomp/Algorithm.hpp`.

The algorithm registry is not just intended as a
collection of compression algorithms, but also as a collection of modular sub
algorithms that the former can make use of for evaluation and customization
purposes.
>TODO: did not understand.

This means that the registry has its own small type system, consisting of "types"
in the form of names for the same class of algorithms, and "names" for each
different algorithm in the same class.
>TODO: did not understand.

In our case, a compressor would have the type `"compressor"` and a suitable name
like `"example_compressor"`.

Putting all this in practice, this is what the code would look like:

~~~ { .cpp }
// [/include/tudocomp/example/ExampleCompressor.hpp]

#ifndef _INCLUDED_EXAMPLE_COMPRESSOR_HPP_
#define _INCLUDED_EXAMPLE_COMPRESSOR_HPP_

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

namespace tudocomp {

class ExampleCompressor: public Compressor {
private:
    // ...
public:
    inline static Meta meta() {
        Meta m("compressor", "example_compressor",
               "This is an example compressor.");
        // ...
        return m;
    }

    inline ExampleCompressor(Env&& env): Compressor(std::move(env)) {
        // ...
    }

    inline virtual void compress(Input& input, Output& output) override {
        // ...
    }
    inline virtual void decompress(Input& input, Output& output) override {
        // ...
    }
};

}

#endif
~~~

Walking through the code, we

- put the new code in the `tudocomp` namespace to not pollute the global one.
- make `meta()` return the required metadata `type: "compressor"`,
  `name: "example_compressor"`, and a documentation string.
- move the `env` parameter to the `Compressor` constructor.

### Input and Output

Next we look at the `compress` and `decompress` methods, which are the entry
points for the actual compression algorithm.

(De)Compressing processes data by reading bytes from an input source,
and outputting bytes to the output target. This happens through the `Input` and
`Output` types, which are abstractions to support different kinds of sources
and targets internally, like files, memory locations, or stdin and stdout.

`Input` allows two different ways to access the input bytes: Either as a
random-access `tudocomp::View` to the whole input, or as a `std::istream` for
linearly processing the bytes.

This separation exists to support both online and offline algorithms.
The former type of algorithm only needs linear access to a current position in the text (it processes the input subsequently character by character).
The latter type requires random access to the whole input at the cost of having the input entirely loaded into memory.

A `View` behaves like a read-only `std::vector<uint8_t>` or `std::string`,
and represents a "window" into a section of memory without requiring
an additional copy of the data. Specifically, this means that copying an
instance of a `View` only copies one pointer and one integer;
thus you can cheaply create sub-views into parts of it:

~~~ { .cpp }
inline virtual void compress(Input& input, Output& output) override {
    auto view = input.as_view();

    auto view2 = view; // Only copies a pointer
    CHECK(view2 == "foobar");
    auto sub_view = view.substr(1, 5); // Only increments a pointer and length
    CHECK(sub_view == "ooba");

    for (size_t i = 0; i < view.size(); i++) {
        uint8_t c = view[i];
        // ...
    }
}
~~~

This allows it to keep the input loaded into memory _just once_.
All further processing consists of manipulating pointers and indices.

In contrast, this is what accessing the input as a `std::istream`
looks like:

~~~ { .cpp }
inline virtual void compress(Input& input, Output& output) override {
    auto stream = input.as_stream();

    char c;
    while(stream.get(c)) {
        // for each char c...
    }
}
~~~

Alternatively, the stream interface also offers an iterator interface:

~~~ { .cpp }
inline virtual void compress(Input& input, Output& output) override {
    auto stream = input.as_stream();

    for(uint8_t c : stream) {
        // for each char c...
    }
}
~~~


`Output` is similar, but only provides the stream interface since
buffering can be done manually:

~~~ { .cpp }
inline virtual void compress(Input& input, Output& output) override {
    auto stream = output.as_stream();
    stream << "foo";
}
~~~

> Note: The framework assumes that a `char` is a 8 bit byte,
> and will use the unsigned `uint8_t` byte type for input/output data unless
> the use of std library types like streams require the use of `char`.

### An example implementation

As an example, we provide here a simple run length encoding compressor that
replaces consecutive runs of the same byte with an integer.
E.g., we encode `"abcccccccde"` as `"abc%6%de"` by replacing the substring `"ccccccc"` consisting of seven subsequent `c`s with `"c%6%"`
to indicate that the previous `'c'` should be repeated six times.

~~~ { .cpp }
inline virtual void compress(Input& input, Output& output) override {
    auto istream = input.as_stream();
    auto ostream = output.as_stream();

    char last;
    char current;
    size_t counter = 0;

    // Use a lambda function here to keep the code DRY
    auto emit_run = [&]() {
        if (counter > 3) {
            // Only replace if we actually get shorter
            ostream << last << '%' << counter << '%';
        } else {
            // Else output chars as normal
            for (size_t i = 0; i <= counter; i++) {
                ostream << last;
            }
        }
    };

    // Try to get first char
    // if false then input == "" and we exit the function
    if (istream.get(last)) {
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
    }
}

inline virtual void decompress(Input& input, Output& output) override {
    // Use the input as a memory view here, just to have both variants used
    auto iview = input.as_view();
    auto ostream = output.as_stream();

    char last = '?';

    for (size_t i = 0; i < iview.size(); i++) {
        if (iview[i] == '%') {
            size_t counter = 0;
            for (i++; i < iview.size(); i++) {
                if (iview[i] == '%') {
                    break;
                } else {
                    // parse the number
                    counter *= 10;
                    counter += (iview[i] - '0');
                }
            }
            for (size_t x = 0; x < counter; x++) {
                ostream << last;
            }
        } else {
            ostream << iview[i];
            last = iview[i];
        }
    }
}
~~~

> Exercise: Implement the decompress-method such that it uses the input as a stream.

> Note: For simplicity, we don't address a few bugs hiding in this
> implementation, most notably the fact that this code would stumble over the
> symbol `'%'` used in the actual input text.

## Tests

After having an implementation of a compressor, we want to
check whether it is working correctly. For that, the framework provides unit
testing with the [gtest](https://github.com/google/googletest) library.
The tests are in the directory `/test`.

### File locations

The layout is relatively simple: Tests are grouped in individual `.cpp`
source files, which in turn are registered in the `CMakeLists.txt` file.

Running `make check` (from the build directory) will run all registered files,
while `make <test_name>` will only run the test corresponding
to `<test_name>.cpp`.

We provide permanent and temporary tests:

For the latter, if you only want to quickly check something, say a part of your algorithms,
or some std library types, you can use the pre-defined `sandbox_tests.cpp` file.
Changes to that file are intended to only be local to the current
developer machine, and will not be committed into GIT.
> TODO: first occurrence of GIT. should we add some kind of commit-etiquette before?

Otherwise, for permanent testing, we create a new source file and register it in the `CMakeLists.txt`.
Here, we add the file `/test/example_tests.cpp`...

~~~ { .cpp }
// [/test/example_tests.cpp]

#include "gtest/gtest.h"

TEST(example, test) {
    ASSERT_TRUE(true);
}

~~~

... and register it in the `CMakeLists.txt` with this line:

~~~
run_test(example_tests DEPS ${BASIC_DEPS})
~~~

Now, if you do `make example_tests`, you should get a test run with
one successful test.

### Adding tests

Next, we will add some real tests to check that the compressor works correctly.

We start with one that manually constructs the `Input` and `Output`
handles and the compressor, and then checks that the result is as expected:
> TODO: first time 'handle' is used

~~~ { .cpp }
#include <tudocomp/example/ExampleCompressor.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

// ...

TEST(example, compress) {
    auto compressor = tudocomp::create_algo<ExampleCompressor>();

    tudocomp::Input i("abcccccccde");

    std::vector<uint8_t> o_buf;
    tudocomp::Output o(o_buf);

    compressor.compress(i, o);

    std::string o_buf_s(o_buf.begin(), o_buf.end());
    ASSERT_EQ(o_buf_s, "abc%6%de");
}
~~~

We need to use the `create_algo<T>()` helper function to correctly instantiate
the compressor, and then create an `Input` that points to the memory
containing the string `"abcccccccde"`. For the `Output`,
we pass a byte vector that will get filled with the output.

Then we simply call the compressor's `compress` method, and look at the content
of the byte vector afterwards. To make comparisons easier, we convert
the output to an string first, and then use the gtest `ASSERT_EQ` macro.

Although this basic approach could be used for all input/output tests,
it is somewhat verbose and gets complicated if
we additionally want to check whether the result is decompressable, or if
we want to test with different inputs.
For this purpose, there exists a header file in the test directory with a few helper functions:

~~~ { .cpp }
#include "tudocomp_test_util.h"

// ...

TEST(example, roundtrip1) {
    test::roundtrip<ExampleCompressor>("abcccccccde", "abc%6%de");
}
~~~

This will check both that `"abcccccccde"` compresses to `"abc%6%de"`, and
that `"abc%6%de"` decompresses to `"abcccccccde"`.

You can also compare against a byte vector, for the usual
case that your encoding is not human readable text:

~~~ { .cpp }
TEST(example, roundtrip2) {
    std::vector<uint8_t> v { 97, 98, 99, 37, 54, 37, 100, 101 };
    test::roundtrip<ExampleCompressor>("abcccccccde", v);
}
~~~

## Statistics

Finally, we show how to add some statistic tracking to the implementation.
The tracking allows to measure data that might be relevant for the evaluation of the
algorithm like the number of computed factors, or the speed.
To this end, the framework offers suitable methods in the `Env` class.

The framework is capable of measuring the time and the used dynamic heap memory during the execution.
To enable the latter capability (tracking the memory), the library `malloc_count` has to linked in;
`malloc_count` tracks the number of allocated bytes; it is unaware for what the memory is used specifically.
> TODO: Hat Patrick nicht seinen eigenen malloc_count geschrieben?

We first present a set of tracking methods that divide the algorithm
into different, possibly nesting, phases:

### Phases

~~~ { .cpp }
// ... inside an algorithm ...
env().begin_stat_phase("Phase 1");
    some_expensive_operation();
env().end_stat_phase();
env().begin_stat_phase("Phase 2");
    env().begin_stat_phase("Phase 2.1");
        some_other_expensive_operation();
    env().end_stat_phase();
    env().begin_stat_phase("Phase 2.2");
        some_expensive_sub_operation();
    env().end_stat_phase();
env().end_stat_phase();
~~~

In the case of our run length encoding example, all the work happens in a single loop.
This makes it hard to assess different phases.
Moreover, we are testing it with inputs that are so short that the algorithms
finishes nearly instantaneously (on a commodity computer).

For didactic purposes, we cheat a little:
We add a few fake phases, make all phases waste runtime by letting the
program simply sleep for a few seconds, and add a fake big memory allocation:

~~~ { .cpp }
#include <chrono>
#include <thread>

// ...

inline virtual void compress(Input& input, Output& output) override {
    env().begin_stat_phase("init");
        auto istream = input.as_stream();
        // ...
        std::this_thread::sleep_for(std::chrono::seconds(1));
    env().end_stat_phase();

    env().begin_stat_phase("run length encoding");
        istream.get(last);
        while(istream.get(current)) {
        // ...
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // add a fake memory allocation
        {
            std::vector<uint8_t> v(100 * 1024, 42); // 100 KiB of the byte 42
        }
    env().end_stat_phase();

    env().begin_stat_phase("nothing");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    env().end_stat_phase();
}
~~~

If we run `make example_tests` again, we will notice that each compression
roughly takes 3 seconds, presumably each of those is spent in a different phase.

### Web Service

Analyzing the collected statistics can be done by the
web service [here](http://dacit.cs.uni-dortmund.de/dinklage/stat/).
The web service visualizes statistic data given in form of a JSON file.

We can retrieve the required JSON file from the `Env` instance.
To this end, we add this line to our written compressor test to
get it printed to the terminal:

~~~ { .cpp }
std::cout << compressor.env().finish_stats().to_json() << "\n";
~~~

The command `make example_tests` will print JSON data looking something like this:

~~~
{
    "title": "root",
    "timeStart": 167494201,
    "timeEnd": 167497202,
    "memOff": 0,
    "memPeak": 103464,
    "memFinal": -252,
    "stats": [],
    "sub": [
        {
            "title": "init",
            "timeStart": 167494201,
            "timeEnd": 167495201,
            "memOff": 124,
            "memPeak": 31416,
            "memFinal": 912,
            "stats": [],
            "sub": []
        },
        {
            "title": "run length encoding",
            "timeStart": 167495201,
            "timeEnd": 167496202,
            "memOff": 1076,
            "memPeak": 102388,
            "memFinal": -12,
            "stats": [],
            "sub": []
        },
        {
            "title": "nothing",
            "timeStart": 167496202,
            "timeEnd": 167497202,
            "memOff": 1044,
            "memPeak": 30504,
            "memFinal": 0,
            "stats": [],
            "sub": []
        }
    ]
}
~~~

If we paste this into the website we will see a bar diagram highlighting the memory profile and the runtime of all three phases.

### Statistics

You can also log individual data points during the run of the algorithm (e.g., logging the number of factors)
For this purpose, the `Env` class provides the `log_stat()` method.

In our example, we will log two statistics: the maximum run length, and the amount of repetitions of the same character (longer than three):

~~~ { .cpp }
inline virtual void compress(Input& input, Output& output) override {
    // ...

    size_t stat_max_repeat_counter = 0;
    size_t stat_count_repeat_segments = 0;

    // Use a lambda here to keep the code DRY
    auto emit_run = [&]() {
        if (counter > 3) {
            // Only replace if we actually get shorter
            ostream << last << '%' << counter << '%';
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

    // ...

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

    // ...

    env().end_stat_phase();

    // ...
}
~~~
> TODO: would be cool if you can highlight the changes like with `diff`

Pasting the JSON output of this in the website will make the two added
statistics appear in the tool tip of the middle phase.

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
> TODO: Why do you emphasize on "native" code?

In order to enable these optimizations, the compiler needs to know
at compile time what function or method of what object will be called.
For standalone functions, it knows this if the function is defined in the
same
`*.cpp` file, or exists with a `inline` annotation in a header
(hence the header-only design of the library).
> TODO: why is inline necessary? This is only a compiler hint

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
>TODO: Do you mean that it does not mean that this does not cause a default template parameter like <T = ActualType>

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
> TODO: cursor undef

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
> TODO: not really clear

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
> TODO: where is the function body?

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

> Note: The tool will prevent accidental overwrites if a file with the
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

> There is also a separate, experimental support for measuring the memory
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

> Note that the make target can be aborted at any time and will resume
> with what it had already downloaded the next time it gets invoked.

The downloaded files can be found in the __source__ directory
of the project, under `/datasets/download`.

This example will use some of those texts.

### Compare Workspace

At this point we already can build the compare tool
and invoke it with a config file.
But this is a bit cumbersome, because:

- The input files and the compare tool can have complex file paths.
- The compare files dumps log files and compression artifacts to the current
  working directory, so it should run in its own directory.
  > TODO: what are compare files dumps log files?
- Changes to the tudocomp codebase will not automatically be recognized
  by the tool.

To address these (arguably minor) concerns the build system offers the target
`compare_workspace`. It will create the directory `/build/compare_workspace`,
and fill it with a short shell script `run.sh` that:

- Invokes the make targets `tudocomp_driver`, `compare_tool` and
  `compare_workspace` to ensure that the built code is up to date.
- Locally appends the binary directory of Tudocomp to`$PATH` such that it does not have to be
  installed or be invoked with the relative/full path.
> TODO: what is "it"?
- Defines an environment variable `$DATASETS` pointing
  to the dataset directory.
- Delegates to the `compare_tool` with the environment
  defined above.
> TODO: what will be delegated? The environment variables? Parameters like below?

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
