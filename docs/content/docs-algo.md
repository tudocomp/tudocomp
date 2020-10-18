@page algo Algorithms

# Algorithms
The abstract class @ref tdc::Algorithm plays a central role in tudocomp
by providing a base for the modular system of compressors, encoders
and so forth. It is the base type for classes that may be registered
in the registry (see @ref registry), which allows them to be
instantiated and parameterized from strings at runtime, e.g., via the
command line. For this, Algorithms provide meta information, which is
described on the @subpage meta page.

#### Configurations.
Algorithms may be parameterized by primitive values (e.g., a maximum
load factor for hash tables) or by sub-algorithms -- strategies (see
below). These parameters are defined in the Algorithm's meta
information. At runtime, parameters are accessed via the Algorithm's
configuration (@ref tdc::Algorithm::config()), which holds the parameter
values parameters used when the Algorithm was instantiated. The
@ref meta page contains examples.

An Algorithm receives its configuration via its main constructor
(@ref tdc::Algorithm::Algorithm(tdc::Config&&)).

#### Programmatic instantiation.
To instantiate Algorithms programmatically, a configuration
(@ref tdc::Config) needs to be created first. The variants of
@ref tdc::Algorithm::instance simplify this process by providing means
to instantiate Algorithms using their default configuration or
modifications thereof.

As an example, an instance of @ref tdc::LiteralEncoder with Huffman
coding is created as follows:
@code{.cpp}
auto c = Algorithm::instance<LiteralEncoder<HuffmanCoder>>();
@endcode

#### Strategies.
A common pattern found in tudocomp is the
[strategy pattern](https://en.wikipedia.org/wiki/Strategy_pattern),
which allows for modularization of algorithms or sections thereof.
In tudocomp, the pattern is realized using C++ templates, which
minimizes the runtime performance overhead (e.g., compared to the use of
virtual classes). The C++ template parameters are logically bound to
Algorithm parameters, so that strategies are part of an Algorithm's
configuration. More information can be found on the @subpage strategy
page.

#### Core algorithm types.
The following algorithms play a major role in tudocomp:
* @subpage coder -- for integer (and character) encoding and decoding.
* @subpage compressor -- the main motivation behind tudocomp.
* @subpage generator -- for generating test inputs.
