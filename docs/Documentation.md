% TuDoComp Documentation
% Patrick Dinklage
% 2016

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

>TODO

## Library

The library is meant to make the compressor implementations accessible by third
party applications.

All code is written in C++11.

### Inventory

>TODO

### License

>TODO

### Linking

The library comes as a set of C++ headers with no linked binaries in any form.
Alas, in order to use a specific compressor implementation, including the
respective headers suffices.

Note the external dependencies listed below.

### Dependencies

The library has the following external dependencies required for compilation:

* [`Boost`](http://www.boost.org/) (`system`, `program_options` and `filesystem`; 1.55 or later).
* [`sdsl-lite`](https://github.com/simongog/sdsl-lite) (2.03 or later).
* [`glog`](https://github.com/google/glog) (0.34 or later).

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

The framework comes with its own abstract [Input](about:blank) and
[Output](about:blank) layer, which hides the actual source or destination of
the data (e.g. a file or a place in memory) away from the implementation.

>TODO: some words on the paradigm

# Credits

*tudocomp* was created with the help of ...

| Name | Roles |
| ---- | ----- |
| Dinklage, Patrick | core development, research, documentation |
| Köppl, Dominik    | initiation and supervision, advice, research |
| Löbel, Marvin     | core development |

>TODO: literature references? or rather put these in the inventory?
