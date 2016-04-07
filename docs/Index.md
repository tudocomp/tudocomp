% TuDoComp Documentation
% Patrick Dinklage
% 2016

This is the documentation for the **T**echnical **U**niversity of DO**rtmund **COMP**ression Framework.

*tudocomp* is a compression utility that implements a range of data compression
and encoding algorithms. These can be mixed and parameterized with the
following uses in mind:

* Finding the optimal compression strategy for a given input.
* Benchmarking and comparison of compression and encoding algorithms.
* Easy integration of new algorithm implementations.

# Structure

In this chapter, we will have a look at the framework's base structure. The
structure is meant to offer a solid and extensible base for new implementations
to be integrated.

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

> TODO

## Inventory

The following algorithms, categorized by compressor family and task, are
currently implemented in *tudocomp*:

> TODO: include from a generated file?

# Coding Guideline

> TODO
