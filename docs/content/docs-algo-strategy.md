@page strategy Strategies

# Strategies
A common pattern found in tudocomp is the
[strategy pattern](https://en.wikipedia.org/wiki/Strategy_pattern),
which allows for modularization of algorithms or sections thereof.
In tudocomp, the pattern is realized using __C++ templates__, which
minimizes the runtime performance overhead (e.g., compared to the use of
virtual classes).

To give a simple example, consider the @ref tdc::LiteralEncoder class.
It is a simple compressor that simply encodes each character from the
input one by one. The used encoding scheme is the compressor's strategy,
represented by the template parameter type `coder_t`.

Strategies are expected to inherit from @ref tdc::Algorithm. This way,
they may receive sub configurations immediately at the time of
instantiation (see below).

#### Linking C++ template parameters to configurations.
In the LiteralEncoder's meta information, we link the template parameter
to a configuration parameter as follows:

@code{.cpp}
meta.param("coder", "The output encoder.").strategy<coder_t>(TypeDesc("coder"));
@endcode

The link is effectively created by the
@ref tdc::meta::ParamBuilder::strategy function, which is forwarded the
template parameter and binds it to the parameter. It also requires a
@ref tdc::TypeDesc, which defines a constraint: only Algorithms that
have a matching type may be used to substitute the template parameter.
This is checked when a configuration for the Algorithm (LiteralEncoder
in this case) is about to be created.

#### Default configurations for strategies.
It is also possible to assign a default configuration for
strategies. Assume we want to make @ref tdc::HuffmanCoder the default
selection for the `"coder"` parameter, we would declare it by passing
an additional @ref tdc::meta::Meta::Default parameter:
@code{.cpp}
meta.param("coder", "The output encoder.").strategy<coder_t>(TypeDesc("coder"), Meta::Default<HuffmanCoder>());
@endcode

The default value for the strategy's parameter is then the default
configuration of HuffmanCoder.

#### Instantiating strategies.
The configuration of a strategy may be obtained from the using
Algorithm's configuration via the @ref tdc::Config::sub_config
function. This configuration can then be used to instantiate the
strategy. In our example of the LiteralEncoder, the encoder is
instantiated as follows:

@code{.cpp}
typename coder_t::Encoder coder(
    config().sub_config("coder"), // pass the sub configuration
    /* ... further constructor parameters */);
@endcode

Note that in this example, `typename coder_t::Encoder` is the actual
strategy type, i.e., LiteralEncoder expects `coder_t` to have an inner
class by the name of `Encoder`. See @ref coder for more information on
this pattern.
