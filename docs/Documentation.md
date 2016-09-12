# Abstract

This is the documentation for the **T**echnical **U**niversity of **DO**rtmund **COMP**ression Framework.

*tudocomp* is a compression utility that implements a range of data compression
and encoding algorithms. These can be mixed and parameterized with the
following uses in mind:

* Finding the optimal compression strategy for a given input.
* Benchmarking and comparison of compression and encoding algorithms.
* Easy integration of new algorithm implementations.

# Project Structure

In this chapter, we will have a look at the framework's base structure. The
structure is meant to offer a solid and extensible base for new implementations
to be integrated.

## Library and driver

The project is made of two major components: the compression *library*
(*tudocomp*) and the framework utility, called the *driver*. The library
contains the core interfaces and provides implementations of various
compressors; the driver provides the interface for the user in the form of an
executable.

The driver uses a *registry* of compressors, which acts as the link between the
driver and the library. The library is meant to be a fully functional component
on its own, however, so that third party applications can make use of the
provided compressors.

## Compressor Families

The *compressor families* form the topmost abstraction level in the framework.
Any compression or encoding algorithm belongs to a certain family.

For instance, the family *lzss* contains various compressors that factorize
the input and different strategies to encode the resulting string of
remaining symbols and the produced factors. This includes the classic
*Lempel-Ziv-Storer-Szymanski* algorithm itself.

## Compressors and Modules

A *compressor*, in terms of this framework, transforms a given input to an
output. It forms an entry point for the utility, which needs to be invoked
with a descriptor pointing at the compressor to use for the input.

Compressors are required to implement a decompressor for any produced output
that can restore the original input losslessly. Apart from this, there are no
strict rules as to *what* transformation occurs.

Of course, the goal is to compress data, and data compression is often achieved
in multiple steps. The idea behind compressors is that they are *modular* and
its modules are interchangeable.

As an example, any factor-based compressor produces a set of factors and
encodes these factors and the remaining, unfactorized raw input. This already
offers the chance for the following modularization:

1. The *factorizer* that produces the factors,
2. a *factor encoder* that encodes these factors to the output and
3. a *raw symbol encoder* that encodes the remaining, unfactorized input.

For each of these tasks, there can be different strategies. For instance, we
can factorize the input using a classic sliding window approach, but we could
as well do it using lcp tables. Factors could be encoded directly with fixed
bit lengths, or we could use a variable-length approach such as Huffman code.
The same goes for the raw symbol encoder.

The idea behind this framework is that as many individual steps as possible for
each compression family are modularized and interchangeable. This way, the
user (or ultimately an artificial intelligence) can mix and match algorithms to
find the optimal compression strategy for a given input.

As much behaviour as possible should be controllable by the user. However,
this should create as little performance overhead as possible.

# Usage

## Framework utility

The main executable, `tudocomp_driver`, is used as a command-line tool that
contains with the necessary usage information.

## Library

The library is meant to make the compressor implementations accessible by third
party applications.

It comes as a set of `C++` headers, no binary objects need to be built. Alas,
in order to use a specific compressor implementation, including the respective
headers suffices.

Please refer to the [Doxygen documentation](about:blank) for an overview over
the available compression and coding implementations.

Please also note the library's [dependencies](#dependencies) and that all code
is written in `C++11`.

### Dependencies {#dependencies}

The framework is built using [CMake](https://cmake.org) (2.8 or later). It is
written in `C++11` and only tested against the `gcc` compiler family.

It has the following external dependencies:

* [Succinct Data Structure Library](https://github.com/simongog/sdsl-lite)
  (2.1 or later).
* [`gflags`](https://gflags.github.io/gflags) (2.1.2 or later).
* [`glog`](https://github.com/google/glog) (0.34 or later).

Additionally, the tests require
[Google Test](https://github.com/google/googletest) (1.7.0 or later).

The build scripts will either find these dependencies on the build system or
automatically download and build them from their official repositories.

For building the documentation, the following tools are required:

* LATEX (specifically the `pdflatex` component)
* [Pandoc](http://pandoc.org)
* [Doxygen](http://doxygen.org)

### Building on Windows

On Windows, only [Cygwin](https://www.cygwin.com/) is currently supported as a
build platform.

>TODO: some notes on what has to be done to get it working

### License

>TODO

# Coding Guideline

This chapter describes how the code is structured on a file level and what
steps are necessary to extend the framework by additional compressor. It also
presents some techniques and standards that have been picked for a
performance-neutral modularization of compressors.

For detailled information on types and functions, please refer to the
[Doxygen documentation](about:blank).

## The Compressor interface

The [`Compressor`](about:blank) class is the foundation for the compression and
decompression cycle of the framework. It acts as a purely virtual interface for
actual compressors to inherit from.

It defines the two central methods for the compression cycle: `compress` and
`decompress`. These are responsible for transforming any input to an output
so that the following (pseudocode) statement is true for any input:

    decompress(compress(input)) == input

In other words, the transformation must be losslessly reversible by the same
compressor class.

When compressing, the framework will ensure that additional information will be
stored in the output so that at a later point, it can detect what compressor
needs to be used to decompress it. This is not the compressor's responsibility.

## The Registry

The [Registry](about:blank) keeps information about available compressor
implementations and thus builds the link between the library and the driver.

Registered compressors will be made available to the user in the utility
(including the listing in the help output). The registry is also used to
identify what compressor to use to decompress an encoded input.

## Input and Output

The framework comes with its own abstract [`Input`](about:blank) and
[`Output`](about:blank) layer, which hides the actual source or destination of
the data (e.g. a file or a place in memory).

`Input` can be used as a *stream* or as a *view*.

The *stream* approach represents the classic technique of reading character by
character. The current state of the stream (ie the reading position) can be
retained by using the copy constructor - this allows "rewinding".

Note that currently, streaming from an `std::istream` is not supported (such streams are buffered
before made available as an `Input`. Streaming from a file works fine, however.

A *view* provides random access on the input. This way, the input acts like an
array of characters.

The `Output` only works stream-based.

### Bitwise input and output

The [`BitIStream`](about:blank) and [`BitOStream`](about:blank) classes provide
wrappers for bitwise reading and writing.

Additionally to operations for reading and writing single bits and fixed-width
integers, they support *compressed integers*. These can be used to minimize
the effective bit width of small values in large value ranges.

Let $b \geq 1$ be the *block width* in bits. A compressed integer is a bit
string of the form $([0|1][0|1]^b)^+$ . The semantics are that the bit
string for the represented value is split into blocks of $b$ bits, starting
with the $b$ lowest bits. Each block is preceded by `1` in case a higher,
non-zero block follows, or `0` if it is the block representing the highest
bits. The lowest block, even if zero, is always stored.

As an example, consider $b=3$. The compressed integer for $v = 42_d = 101010_b$
would be `1 010 0 101`. The value $v' = 3_d = 11_b$ would be represented as
`0 011`.

# Tutorial

This document will walk you through writing an compressor using
most of the provided functionality of the framework. It will discuss the
following topics:

- How to build the framework.
- Where and how to start adding files into the framework.
- Writing a simple compressor by implementing the `Compressor`
  interface.
- Adding tests for debugging and checking your code.
- Adding basic statistic tracking to your code,
  for keeping track of where time and memory
  is being spent.
- Adding runtime options to your code to select different
  behavior.
- Adding compiletime (template) options to your code to select
  different behavior that due to performance reasons
  should not be selected at runtime.
- Registering your code in the registry.
- Using the `tudocomp_driver` command line tool
  with your algorithm.
- Using the `compare_tool` to compare compression ratio and
  runtime between other algorithms or text files.

You may also refer to an [UML overview](#uml-class-overview) of the framework.

## Building the Framework

Tudocomp is set up as a cmake project, which means that
all metadata for building it is encoded in
`CMakeLists.txt` files through the source tree.

If you are unfamiliar with cmake, note that it works by generating build
scripts for the platform it is run on. In the case of Tudocomp, you will
likely use it in a Linux or a Windows-Cygwin environment, for which
cmake will generate makefiles to be used by `make`.

We will also assume the use of the command line through this tutorial.
There exist graphical interfaces for some of the used tools,
but those are outside the scope of this document.

So, starting with a copy of the Tudocomp sources, our first step
is to generate the build environment.
The usual setup for this is to create a "build" directory inside the
Tudocomp sources, and invoke `cmake` inside it with the parent directory as
an argument:

~~~
.../tudocomp> mkdir build
.../tudocomp> cd build
.../build> cmake ..
~~~

If it the output ends with `-- Build files have been written to: [...]`, then the command was successful.
Otherwise your probably have to install a few dependencies first.

Note that `cmake ..` configures the build in debugging mode per default,
which means the compiled C++ code will not be optimized and instead will
have additional debugging code enabled.

This is usually wanted for development, but is unhelpful for comparing performance, so if you want to compile your code optimized you can explicitly tell cmake to configure in "Release" mode:

~~~
.../build> cmake -DCMAKE_BUILD_TYPE=Release ..
~~~

And if ou want to be explicit with using the Debug mode, you can also do this:

~~~
.../build> cmake -DCMAKE_BUILD_TYPE=Debug ..
~~~

The next step is to actually build the library.
This could take a while at the first time,
since it might have to download and compile a few missing dependencies:

~~~
.../build> make
[...] lots of output [...]
~~~

We can then run the testsuite and generate the docs:

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

The source tree contains a few different things: The framework library, the command line driver for it, the algorithm registry used by the former, the test suite and the compare tool.

The `.cpp` files for these parts lives in their own directories
in `/src`, and the header files live in their own directories
in `/include`.

The framework itself is organized as a header-only C++ library,
which means that all code lives in header files, and none in `.cpp` source files.

This is done for two reasons:

- Performance: If the code lives in headers, then it can get
  optimized better since the compiler can always see all of it, and
  thus can do optimizations it could not do if parts of the code would
  only be linked together from another compiled `.cpp` file.
- Convenience: A lot of the code is templated, and thus would live in
  in a header anyway. By also moving all the code that would not
  _have_ to be in a header there, we get rid of the typcial C++ way
  of splitting up all the code between header and `.cpp` files.

> Note: This is not a hard design decision, and might change again
> depending on how the framework develops.

If there is uncertainty about how header-only C++ code works,
note that the basic idea is
that any function or other definition not defined as a template
gets annotated with `inline`, which allows putting its
implementation directly in the header.

So, as a start for adding a new compressor
to the framework, we would create a new header file somewhere in `/include/tudocomp/` or in a new subdirectory in it.

For this tutorial, we will pick `/include/tudocomp/example/`.

> Note: All example code introduced in this Tutorial exists
> as actual example files in the source code of the framework.

## A Simple Compressor

### The interface

Adding a new compressor starts with
implementing the basic interface for a Tudocomp compressor.

Since the framework is a template heavy library, this involves
having the right members and properties in your class, rather than just
inheriting from an abstract interface.

Specifically, to implement a Tudocomp compressor
the following interface needs to hold for the class:

- The class needs to publicly inherit from the `tudocomp::Compressor`
  interface and implement its virtual methods.
- The class needs a constructor with the signature `(tudocomp::Env&&)` that
  delegates to the `Compressor` constructor.
- The class needs to have a static member function `meta()`
  that returns an instance of the `tudocomp::Meta` type.

`Env` is a helper class for accessing the environment around the algorithm; it
gives you access to possible runtime flags, and the
statistic tracking framework. Conceptually, a compressor owns its environment,
which is why the constructor takes ownership through the rvalue reference. It
should always just be delegated to the base class constructor, since that will
take care about storing it and providing access to it with the `.env()` getter.

`Meta` contains metadata that is important for the algorithm to identify itself,
and to declare what additional options it might take. It is mainly used by the
registry to tell apart different Algorithms at run time, but is also needed for
correct construction of your class since the environment will have
to be initialized in an algorithm-specific way by the framework.

For more details on those types see the API docs, or the headers
`tudocomp/Compressor.hpp`, `tudocomp/Env.hpp` and `tudocomp/Algorithm.hpp`.

The algorithm registry is not just intended as a
collection of compression algorithms, but also as a collection of modular sub
algorithms that the former can make use of for evaluation and customization
purposes.

This means that the registry has its own small type system, consisting of "types"
in the form of names for the same class of algorithms, and "names" for each
different algorithm in the same class.

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
               "This is a example compressor.");
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
and targets internally, like for example files, memory locations, or stdin and
stdout.

`Input` allows two different ways to access the input bytes: Either as a
random-access `tudocomp::View` to the whole input, or as a `std::istream` for
linearily walking through the bytes.

This separation exists to support both online algorithms,
that only need linear access to a current position in the text, and
offline algorithms, that require random access to the whole input
at the cost of having it entirely loaded into memory.

A `View` behaves like a read-only `std::vector<uint8_t>` or `std::string`,
and represents a "window" into a section of memory without requiring
an additional copy of the data. Specifically, this means that copying an
instance of a `View` only copies some pointer, and
that you can cheaply create sub-views into parts of it from it:

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

This allows it to keep the input loaded into memory just once,
with all further processing just consisting of manipulating
pointer and indices into it.

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

As an example, here is a simple run length encoding compressor that only
replaces consecutive runs of the same byte with an integer.
Eg, `"abcccccccde"` would be
encoded as `"abc%6%de"` to indicate that the
previous `'c'` should be repeated 6 times.

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

> Note: For simplicity, we don't address a few bugs hiding in this
> implementation, most notably the fact that this code would stumble over the
> symbol `'%'` used in the actual input text.

## Tests

Now that we have a basic implementation of a compressor, we need to actually
check whether it is working correctly. For that, the framework provides unit
testing with the [gtest](https://github.com/google/googletest) library, and can be found under `/test`.

### File locations

The layout there is relativly simple: Tests are grouped in individual `.cpp`
source files, which in turn are registered in the `CMakeLists.txt` file.

Running `make check` (from the build directory) will run all registered files,
while `make <test_name>` will only run the test file corresponding
to `<test_name>.cpp`.

From there, there are two options:

If you only want to quickly check something, say part of your algorithms,
or some std library types, you can use the pre-defined `sandbox_tests.cpp` file.
Changes to that file are intended to only be local to the current
developer machine, and will not be committed into GIT.

Otherwise, for permanent testing, we need to add one or more source
files for tests that correspond to our addition to the framework.

We'll add the file `/test/example_tests.cpp` here...

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

Next, we'll add some real tests to check that the compressor works correctly.

We start with one that manually constructs the `Input` and `Output`
handles and the compressor, and then checks that the result is as expected:

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
containing the string `"abcccccccde"`. For the `Output` on the other hand
we pass in a reference to a byte vector that will get filled with the output.

Then we simply call the compressor's `compress` method and look at the content
of the byte vector afterwards. To make comparing it easier, we convert
it to an string first, and then use the gtest `ASSERT_EQ` macro.

Though this basic approach could be used for all input-output tests,
it is somewhat verbose and gets complicated fast if
we also want to check if the result can be decompressed again or
want to check with many different inputs, so there exists a header file in
the test directory with a few helper functions:

~~~ { .cpp }
#include "tudocomp_test_util.h"

// ...

TEST(example, roundtrip1) {
    test::roundtrip<ExampleCompressor>("abcccccccde", "abc%6%de");
}
~~~

This will check both that `"abcccccccde"` compresses to `"abc%6%de"`, and
that `"abc%6%de"` decompresses to `"abcccccccde"`.

Note that you can also compare against a byte vector, for the usual
case that your encoding is not human readable text:

~~~ { .cpp }
TEST(example, roundtrip2) {
    std::vector<uint8_t> v { 97, 98, 99, 37, 54, 37, 100, 101 };
    test::roundtrip<ExampleCompressor>("abcccccccde", v);
}
~~~

## Statistics

Now we'll look at how to add some statistic tracking to the implementation,
to allow measuring of data that might be relevant for the evaluation of the
algorithm. For this, the framework offers suitable methods on the
`Env` type.

The framework in general is capable of measuring the time and, if the
`malloc_count` library is linked in, the used dynamic heap memory of a run.
It can't know what the memory is used for specifically though,
so the first set of tracking methods is about dividing the run time
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

In the case of our example, all the work happens in a single loop though,
so there aren't really different phases you could make out.
Also, we are testing it with short enough inputs that the algorithms
basically finishes instantaneously.

So, for the sake of showing how it works, we cheat a bit:
We add a few fake phases, make all phases take up run time by letting the
program simply sleep for a few seconds, and also add a fake big memory allocation:

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

Now, if we run `make example_tests` again, we will notice that each compression
roughly takes 3 seconds, presumably each of which is spend in a different phase.

### Web service

But how do we actually look at this data? For this, the framework provides a
web
service [here](http://dacit.cs.uni-dortmund.de/dinklage/stat/) for
visualizing statistic data given in form of a JSON document.

Said JSON data can be retrieved from an `Env` instance.
For example, we can add this line to our manual compressor test to
get it printed out to the terminal:

~~~ { .cpp }
std::cout << compressor.env().finish_stats().to_json() << "\n";
~~~

Which should `make example_tests` print out JSON data looking something like this:

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

If we paste this into the website we'll see a bar diagram highlighting the memory profile and run time of the 3 phases.

### Statistics

You can also log individual data points during the run of the algorithm,
for example to log the maximum amount of factors encountered, or some maximum
value reached. For this, the `Env` struct provides the `log_stat()` method.

In our example, we'll log two statistics; the maximum run length encountered, and the amount of repeat sequences encountered:

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

Pasting the JSON output of this in the website will make the two added
statistics appear in the tool tip of the middle phase.

## Options

Next we'll look at how to add support for runtime flags and options. These are
useful in cases where you want to evaluate your implementation with different
parameters, for example a different dictionary size, a different threshold
value, enabling of optional code paths, etc.

For our example, we'll add two options: one to disable the thread sleep
calls, and one to change the escape symbol used by the encoding.

To add these options to your code, you need to first declare them in the
Algorithm's `meta()` method:

~~~ { .cpp }
inline static Meta meta() {
    Meta m("compressor", "example_compressor",
           "This is a example compressor.");
    m.option("debug_sleep").dynamic("false");
    m.option("escape_symbol").dynamic("%");
    return m;
}
~~~

`m.option("...")` declares an option with a specific name,
and the `.dynamic()` indicates that these are runtime options,
as opposed to "templated" ones (We'll look at those in the next section).

The argument of `dynamic()` is optional, and provides a default value if
given. In our case, we want to default to no sleep calls, and using the
`%` symbol.

We then can go ahead and starting querying the value of these
options in the implementation, by using
the `env().option("...").as_...` family of methods:

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

## Option syntax and testing

Now we need to test these changes. For this, we need to briefly look
at the command line syntax used for specifying an algorithm and its options.

Algorithms are specified with an id string that concisely
specifies all (sub)algorithms and options it takes. The syntax works
like a function call that supports keyword arguments and default arguments.

As an example, say we have an algorithm `foo` that accepts a string option
`opt1` with default value `'qux'` and a string option `opt2` with default
value `'bar'`, then all these are valid id strings:

 Id string                            | Full meaning
--------------------------------------|------------
 `foo`                                | `foo(opt1='qux',opt2='bar')`
 `foo()`                              | `foo(opt1='qux',opt2='bar')`
 `foo('x')`                           | `foo(opt1='x',  opt2='bar')`
 `foo('x', 'yz')`                     | `foo(opt1='x',  opt2='yz' )`
 `foo('123', opt2='456')`             | `foo(opt1='123',opt2='456')`
 `foo(opt2 = 'asd', opt1 = 'xkc')  `  | `foo(opt1='xkc',opt2='asd')`
 `foo(opt2 = '...')`                  | `foo(opt1='qux',opt2='...')`

The reason the syntax is so complex is that the framework wants to support
many different algorithms with many options, and thus needs a concise
notation for specifying a concrete combination of them that works with
default values.

Now, if we go back to our example code, both the
`tudocomp::create_algo()` and the `test::roundtrip()` functions
accept an optional string argument which accepts option values
in the same syntax as above, though without the enclosing algorithms name:

~~~ { .cpp }
tudocomp::create_algo<ExampleCompressor>("'/', true");

test::roundtrip<ExampleCompressor>("abcccccccde", "abc-6-de",
                                   "escape_symbol = '-'");
test::roundtrip<ExampleCompressor>("abcccccccde", "abc%6%de",
                                   "debug_sleep = true");
~~~

You can see this in action in the `compress_stats_options` example test.

## Template Options

Normal runtime options as discussed above are useful,
but they have one aspect that makes them not universally applicable:
Their value is determined at runtime.

This is often fine, but there are scenarios where this can hamper
performance for one simple reason: If the value is only know at
runtime, the compiler can not make optimization decisions
based on its value at compiletime.

In the context of the framework, this issue can mostly
be encountered if your algorithm starts to have different sub algorithms,
like for example different binary encoding schemes, or different algorithms
for finding redundancy in the input text.

Lets look at an example of what the actual performance problem
can be in that case:

Say in your compressor you produce some kind
of compression data in a loop, and then process and encode it
with an sub algorithm that is selected by an runtime option.
The code would look somewhat like this:

~~~ { .cpp }
class SubAlgorithmInterface {
    virtual void process_and_encode(...);
}

std::unique_ptr<SubAlgorithmInterface> sub_algo;
std::string sub_algo_name = env().option("...").as_string();

// Select the algorithm to use
if sub_algo_name == "algo1" {
    sub_algo = std::make_unique<SubAlgo1>();
} else if sub_algo_name == "algo2" {
    sub_algo = std::make_unique<SubAlgo2>();
}

for(auto byte : input.as_stream()) {
    // ... process(byte)
    data = ...;
    sub_algo.process_and_encode(data);
    // ...
}
~~~

The problem with this code is the `process_and_encode()` call inside the
loop.
Because the compiler does not know which of the algorithms will actually be
selected at run time, it needs to emit native code that does a virtual call
(that is, it loads the function pointer for the method from a
[vtable](https://en.wikipedia.org/wiki/Virtual_method_table) and calls it).
Not only can doing this a lot in a tight loop cause overhead,
the compiler is also not able to "merge" the native code of the function body
with the native code of the loop for a more efficient and faster compiled
program.

So, in order to enable these optimizations, the compiler needs to know
at compile time what function or method will be called of what object.
For standalone functions, it knows this if the function is defined in the
same
`*.cpp` file, or exists with a `inline` annotation in a header
(hence the header-only design of the library).

For our example of an algorithm selection, the recommended way is to turn
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

This basically moves the selection of the sub algorithm
up from a runtime lookup to a compiletime decision between
`MyCompressor<SubAlgo1>` and `MyCompressor<SubAlgo2>`.

By compiling code using either of those types, it will generate
native code
for the `SubAlgo1` case, or native code for the `SubAlgo2` case,
in either case having full knowledge of the code inside the loop body, which allows
the optimizations.

Now lets look at how this would work for our example compressor.
Again, we don't really have enough code that actually does something
for a real example, but we can still show the idea by making the
exact way the repeated character is encoded changeable with a sub algorithm.

Because this changes and adds a lot more to the existing example code,
we create and modify a copy of the `ExampleCompressor` source in the
`ExampleCompressor.hpp` file, called `TemplatedExampleCompressor`.
It can be found below the original.

First, we make the class generic and give it a private
field that should hold the concrete encoder:

~~~ { .cpp }
template<typename T>
class TemplatedExampleCompressor: public Compressor {
private:
    // We want to keep a instance of the encoder
    // alive for the same duration as the compressor itself, so
    // keep it as a field
    T m_encoder;
~~~

Note that depending on the design of the algorithm, the class could also
just be instantiated temporary in the `compress()` or `decompress()` methods,
so it doesn't _have_ to be a field of the compressor.

Then, because we want to make the choice of the encoder
visible as an option of the algorithm, we declare
it as an "templated" option in our meta method:

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

Note that we have removed the `escape_symbol` option. That is
because it'll become specific to the actual encoder used.

Just as with `dynamic()`, we also have the option of giving a default
"value" for the `templated()` options - specifically,
it would look like `.templated<T, ActualType>()`.
Note that this will not automatically make this the default for the template
parameter of the class itself, just for the registry and command line tool.

Next, we'll have to initialize the encoder in the constructor of the compressor.
It should again be noted that the actual API of an sub algorithm can be
pretty free form, with almost no restrictions on the amount of methods
and life cycle of the instances themselves.

However, if you want to use the templated parameter as an proper algorithm option
then you should make sure that they inherit from `tudocomp::Algorithm`,
and have an constructor that takes an `Env&&` as first argument delegating
to its constructor.

We'd then initialize it with an new `Env` instance created by
`env().env_for_option("<option_name>")`. This ensures that the options for an
sub algorithm will be delegated to its actual constructor:

~~~ { .cpp }
    inline TemplatedExampleCompressor(Env&& env):
        Compressor(std::move(env)),
        // Initialize it with an `Env` for all its options
        m_encoder(this->env().env_for_option("encoder")) {}
~~~

Then we'll have to modify the actual compression routine to
delegate to the template class at the points where we want different
pluggable behavior.

For this, we define 4 methods that an suitable "ExampleEncoder" need to provide:

- `size_t threshold()`, which returns an integer for the minimum repeat count
that should be replaced for the encoder. For example, if you replace `"aaa..."`
with `a%N%` then that will only reduce the size for repeats >3.
- `void encode_repeat(char last, size_t repeat, io::OutputStream& ostream)`,
for actually emitting the encoding for a char repeat sequence.
- `is_start_of_encoding(const io::InputView& iview, size_t i)`,
for identifying the start of an repeat sequence in the decoder.
- `size_t decode(const io::InputView& iview, size_t& i)`,
for actually decoding a sequence in the decoder (advancing the cursor i in the process).

Again, this interface is an entirely arbitrary example, any algorithm
can define its own one.

So, integrated in the example code, it looks like this:

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

Finally, lets look at the implementation for the actual encoder classes.
For our example, we'll define two different ones, one doing the existing
human readable encoding of repeats (`"aaaa" -> "a%3%"`), and another one
doing a more binary oriented one, where instead of a readable number it just
encodes the length as an byte directly (`"aaaa" -> [97, 255, 3]`, with `255`
being the escape symbol).

Again, for the sake of simplicity, we don't address some of the hidden bugs
like `'%'` or `'\xff'` appearing in the input text, or there being an
repeat longer than what fits in an byte.

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
               "This is a example debug coder, encoding human readable.");
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
               "This is a example bit coder, encoding as a binary integer.");
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

We also want to test that the templated version of the compressor works as
intended. For that, we can just do the same testing as before:

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

Note that template options will set the corresponding parameters
in an option string to an fixed value, so if you explicitly set some options
you either have to leave off the option parameter belonging
to the template parameter, or give it the same value as indicated by it;
otherwise you will get an error:

~~~ { .cpp }
// ok
tudocomp::create_algo<TemplatedExampleCompressor<ExampleDebugCoder>>("debug(escape_symbol = '-')");
// error
tudocomp::create_algo<TemplatedExampleCompressor<ExampleDebugCoder>>("bit(escape_byte = '123')");
~~~

## The Registry

Once an compressor implementation exists and has a few unit tests that
verify the basic operation, it needs to be registered in the global
algorithm registry in order to be usable from the command line tool,
and to be automatically picked up by a few automatic unit tests
that run all algorithms with a number of exotic strings.

Doing this is relatively easy: In `src/tudocomp_driver/tudocmp_algorithms.cpp`,
add includes for your compressor headers, and then add new `r.compressor<T>()`
lines for all compressor classes in the function body there.

For the templated version, you need to register the class with
all possible combinations of type parameters it can accept, in order for there
to be an native code instance for all of them.

So, for this example we add these three lines:

~~~ { .cpp }
r.compressor<ExampleCompressor>()
r.compressor<TemplatedExampleCompressor<ExampleDebugCoder>>();
r.compressor<TemplatedExampleCompressor<ExampleBitCoder>>();
~~~

If we now run `make check`
(or directly `make tudocomp_driver_algorithm_matrix_tests`), the new
compressors should be picked up by the "tudocomp_driver_algorithm_matrix_tests"
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

After integrating into the registry, we can start using the algorithm
through the command line tool itself.

The "Driver" (named this way because it serves as an user interface that
drives the underlying tudocomp library) can be compiled with `make tudocomp_driver`, and can be found at
`build/src/tudocomp_driver/tudocomp_driver` afterwards.

As a first step after building it, we can verify that the new compressors
exists by listing all known algorithms:

~~~
.../build> ./src/tudocomp_driver/tudocomp_driver --list
This build supports the following algorithms:

  [Compression algorithms]
  -------------------------------------------------------------------------------------------------------------------------------
  | ...                                                            | ...                                                        |
  -------------------------------------------------------------------------------------------------------------------------------
  | example_compressor(escape_symbol = "%", debug_sleep = "false") | This is a example compressor.                              |
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
    | debug(escape_symbol = "%")                 | This is a example debug coder, encoding human readable.    |
    |   where `escape_symbol` is one of [string] |                                                            |
    -----------------------------------------------------------------------------------------------------------
    | bit(escape_byte = "255")                   | This is a example bit coder, encoding as a binary integer. |
    |   where `escape_byte` is one of [string]   |                                                            |
    -----------------------------------------------------------------------------------------------------------

  ...
~~~

Next we'll actually compress something with the tool. Since this is just
supposed to be a quick test, we'll use an already existing file in the build
directory, the `CMakeCache.txt`:

~~~
.../build> ./src/tudocomp_driver/tudocomp_driver -algorithm "example_compressor" CMakeCache.txt -output ./cache.tdc -stats
~~~

This will create a compressed file `cache.tdc` in the current directory, and
also print some statistics to the command line, including the JSON data that
can be pasted into the previously mentioned web visualizer.

> Note: The tool will prevent accidental overwrites if a file with the
output filename already exists. To disable this, and allow overwriting,
the `-force` option can be used.

The stat output will likely show a very small compression effect, if any at
all, but the file format seems to contain a few sections decorated with
comments containing repeats of the same character, so viewing the contents
of `cache.tdc`, say with `cat cache.tdc | less`, should allow seeing some
encoding sequences in the file.

As a point of reference, on the authors machine it only resulted in a 1% reduction, with 4 actual run length replacements.

For a last check, we can see if decompression works as well:

~~~
.../build> ./src/tudocomp_driver/tudocomp_driver -decompress cache.tdc -output cache.orig.txt
.../build> diff -s CMakeCache.txt cache.orig.txt
Files CMakeCache.txt and cache.orig.txt are identical
~~~

Seems to be the case.

## The Compare Tool

Finally, the framework also provides a rudimentary tool for
automatically running different command line tools on different input text
files.

This "compare_tool" can be used to compare the effects of a compression
algorithm on different classes of input text, or the effects of
different algorithms on the same input files. It can be build with `make compare_tool`.

It does not currently integrate into the frameworks own statistics tracking
though, so it can only easily be used for comparing the run time and
compression ration of different file-command combinations.

> There is also a separate, experimental support for measuring the memory
footprint with the valgrind tool massif, but this does not work well with
external compression programs, and became unneeded for tudocomp itself
with the new memory statistic tracking methods, so it can be considered
deprecated.

The way the tool works is that it gets invoked with
a config file containing a number of comparison profiles,
and a name of one of them, and then executes the profile, printing
the results to stdout as they come in.

A profile consists of a few options, and two lists. One is a list of commands
to run, and one a list of input files to run them on. All the tool then does is
building the Cartesian product of both lists, and executing each combination
in turn, noting time and compressed size.

The config file follows [TOML](https://github.com/toml-lang/toml) syntax,
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

`inputs` contains the list of filenames, and `commands` the list
of command line commands to run. The commands are split into two
strings that will be merged by the tool. The reason for this split
is just so that the first part of the command line can act as a label, while
the second part can contain plumbing like correctly directing the
input and the output, while being irrelevant for identifying the tool and
algorithm itself.

For the command strings there are also two special variables
defined, `$IN` and `$OUT`, that are filled-in by the compare_tool
with file paths to the input/output filename.

The `compare_commands` flag decides how the benchmark runs are grouped.
If it is set to `true`, it will run each command in turn on the same input
file, if it is set to `false` it will run the same command with all input
files:

 `compare_commands = true` | `compare_commands = false`
---------------------------|----------------------------
 `FILE1` run on `COMMAND1`, `COMMAND2` | `COMMAND1` run with `FILE1`, `FILE2`
 `FILE2` run on `COMMAND1`, `COMMAND2` | `COMMAND2` run with `FILE1`, `FILE2`
 ...                                   | ...

### Datasets

Before we look at how to such a config file would look like for our example
compressor, we need some real text examples to compare against.
A good source for them is the [Pizza&Chili Corpus](http://pizzachili.dcc.uchile.cl/texts.html),
which contains a number of different text classes.

The build system provides a target `datasets` that will download a selection
of files from that site. Note that at the point of writing this,
this resulted in 9.4 GiB of files, so make sure you have enough free storage
before doing this:

~~~
.../build> make datasets
~~~

> Note that the make target can be aborted at any time and will resume
> with what it had already downloaded the next time it gets invoked.

After the downloaded, the files can be found in the __source__ directory
of the project, under `/datasets/download`.

This example will use some of those texts.

### Compare Workspace

At this point it would be possible to just build the compare tool
and invoke it with a config file referring to the datasets example
text, but this would still be a bit cumbersome because:

- The text files and compare tool end up with somewhat complex file paths.
- The compare files dumps log files and compression artifacts to the current
  working directory, so it should run in its own directory.
- Changes to the tudocomp codebase will not automatically be picked up
  by the tool.

To address these, arguably minor, concerns the build system offers the target
`compare_workspace`. It will create the directory `/build/compare_workspace`,
and fill it with a short shell script `run.sh` that:

- Invokes the make targets `tudocomp_driver`, `compare_tool` and
  `compare_workspace` to ensure all build code is up to date.
- Locally appends the binary directory of tudocomp to`$PATH` so it doesn't have to be
  installed or be invoked with the full path.
- Defines a environment variable `$DATASETS` pointing
  to the dataset directory.
- And finally delegates to the `compare_tool` with the environment
  defined above.

This enables us to quickly set up a test environment for different algorithms:

~~~
.../build> make compare_workspace
.../build> cd compare_workspace
.../compare_workspace> ./run.sh config.toml example
...
~~~

### An example config file

As the last step of this Tutorial, we'll create an
example compare config file in the workspace directory.

Our goal here is to compare the three compressor variants we had
defined with a few different classes of input: english text,
source code, dna sequences, etc. This requires the datasets.

We create a new textfile `/build/compare_workspace/example_config.toml`, and
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

This defines a config file with the profile `example`
that runs the algorithms `example_compressor`, `templated_example_compressor(bit)`
and `templated_example_compressor(debug)` on 5 different input texts,
grouping the algorithms together since their differences should be compared.

We then invoke this config, and should get an output like this:

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

As we can see, the algorithms are not very effective in either
case, and as expected there is no visible difference between the
first compressor, and the second one with the debug encoder.

We can also toggle the `compare_commands` option to group
by input file instead, to compare how the algorithms perform on different text:

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

As we can see, source code seems to compress best, presumably due
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
