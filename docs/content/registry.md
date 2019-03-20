@page registry Registry

# Registry

The registry is a build configuration script that determines which algorithms
will be compiled and linked into the tudocomp library and command line tool.

Because tudocomp makes heavy use of template parameters, the number of algorithm
easily becomes very large, which impacts build time and binary size. The
registry allows for customizing the feature set for the task at hand, since
*all* algorithms are rarely needed for an application or experiment.

#### Registry scripts.

There are a couple of preset registry scripts located in the `etc/registries`
directory of the project.

The registry script `all_algorithms.py` contains all algorithms implemented in
tudocomp and can be used for reference. Note that it is __not__ advised to use
it for building, because this will result in an excessive build time and binary
file size.

#### Selecting the registry.

> TODO: Which is the default script?

To select a registry script, pass the `-DTDC_REGISTRY=<file>` option to CMake,
where `<file>` points to the registry to be used. The CMake output will always
contain a line informing the user which registry is being used.

#### Customizing the registry.

Registry scripts are written in Python 3. In their core, they set the following
three arrays:

@code{.py}
tdc.compressors =   []
tdc.decompressors = []
tdc.generators =    []
@endcode

These arrays determine the available compressors (@ref tdc::Compressor),
decompressors (@ref tdc::Decompressor) and string generators
(@ref tdc::Generator) that will be compiled, linked and registered for runtime
lookup. Each array contains `AlgorithmConfig` tuples, which provide links to the
corresponding C++ classes and its template parameters.

To give an example, consider the class @ref tdc::LiteralEncoder<coder_t>, which
is a simple compressor that only encodes the input using a @ref tdc::Coder. The
coder is given as the `coder_t` template parameter. It can be added to the
compressor registry as follows:

@code{.py}
tdc.compressors += [
    AlgorithmConfig(name="LiteralEncoder", header="compressors/LiteralEncoder.hpp", sub=[coders])
]
@endcode

As can be seen, the `AlgorithmConfig` tuple accepts three values: `name`
is the name of the C++ class (where the `tdc::` namespace specifier may be
omited) and `header` points to the header file (relative to `include/tudocomp`)
that defines the class. The `sub` value is an array that has one entry for each
template parameter of the class -- one in this case. The entries of `sub` are in
itself arrays of `AlgorithmConfig` tuples: each entry determines the classes
allowed to substitute the template parameters.

Assume that we want to allow the `coder_t` template parameter of
@ref tdc::LiteralEncoder to be substituted by the @ref tdc::HuffmanCoder. Prior
to the registration line above, we define:

@code{.py}
coders = [
    AlgorithmConfig(name="HuffmanCoder", header="coders/HuffmanCoder.hpp")
]
@endcode

Our registry script will now tell the build system that the
template instance `LiteralEncoder<HuffmanCoder>` should be compiled and linked.

When customizing the registry, we recommend using the lightweight `basic.py`
registry as a base and use `all_registries.py` for reference.

#### Registry, algorithm meta information and command line tool.

Using the example registry above, it is possible to use `encode(coder=huff)` as
the algorithm option of the command line tool. The name `encode` and its
parameter `coder` are provided by the meta information of
@ref tdc::LiteralEncoder, the name `huff` is provided by @ref tdc::HuffmanCoder.
See @ref meta and @ref driver for more information.

Contrary to that, attempting to use `encode(coder=gamma)` would result in an
error message. Even though @ref tdc::EliasGammaCode provides the name `gamma`,
it may not have been linked into the binary. Even if it were linked, we never
registered the template instance `LiteralEncoder<EliasGammaCoder>`, so it is
not available in the build. To make it available, an algorithm config for the
elias gamma coder would have to be added to the `coders` list.
