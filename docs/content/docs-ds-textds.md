@page textds Text Index Data Structures

# Text Index Data Structures
The following text index data structures are implemented in tudocomp:
* Suffix arrays (@ref tdc::ds::SUFFIX_ARRAY)
* Inverse suffix arrays (@ref tdc::ds::INVERSE_SUFFIX_ARRAY)
* LCP (longest common prefix) arrays (@ref tdc::ds::LCP_ARRAY)
* Permuted LCP arrays (@ref tdc::ds::PLCP_ARRAY)
* Phi arrays (@ref tdc::ds::PHI_ARRAY)

The @ref tdc::DSManager class provides access to these data structures. It
accepts *providers* as template parameters. Each provider constructs one or
multiple data structures and may require other data structures to be constructed
first -- e.g., a provider for the inverse suffix array typically requires the
suffix array. The `DSManager` orchestrates the construction of these data
structures with the goal of minimizing the RAM usage peak. Said orchestration
is done at compile-time and thus doesn't generate any runtime overhead.

#### Providers
The following providers are available in tudocomp:
* @ref tdc::DivSufSort -- constructs the suffix array using
  [divsufsort](https://github.com/y-256/libdivsufsort).
* @ref tdc::ISAFromSA -- constructs the inverse suffix array from the suffix
  array in plain array representation.
* @ref tdc::SparseISA -- constructs the inverse suffix array from the suffix
  array in a sparse sample-based representation, resulting in lower memory
  consumption but increased access times.
* @ref tdc::PhiFromSA -- constructs the Phi array from the suffix array.
* @ref tdc::PhiAlgorithm -- constructs the permuted LCP array using the Phi
  array.
* @ref tdc::LCPFromPLCP -- constructs the LCP array by permuting the PLCP array.

#### Usage
The following example constructs the suffix and LCP array for an input
(a string or @ref tdc::InputView).

@code{.cpp}
// define data structure manager type
using dsman_t = DSManager<DivSufSort, PhiFromSA, PhiAlgorithm, LCPFromPLCP>;

// instantiate using input
auto dsman = Algorithm::instance<dsman_t>(input);

// construct suffix and LCP array
dsman->template construct<ds::SUFFIX_ARRAY, ds::LCP_ARRAY>();

// retrieve results
auto& sa = dsman->template get<ds::SUFFIX_ARRAY>();
auto& lcp = dsman->template get<ds::LCP_ARRAY>();
@endcode

The types of `sa` and `lcp` depend on the providers. In the above case, they are
of type @ref tdc::DynamicIntVector as given by the @ref tdc::DivSufSort and
@ref tdc::LCPFromPLCP providers.

Note that even though we only request the suffix and LCP array, we need to
give a provider for the PLCP array, because it is required by
@ref tdc::LCPFromPLCP. The PLCP provider, @ref tdc::PhiAlgorithm, requires
the Phi array, so we give @ref tdc::PhiFromSA as well.

#### Troubleshooting
In case there is no provider for a data structure required in the construction
chain, a compile-time error message will be generated from a failed static
assertion as such:
@code
error: static assertion failed: no provider for data structure
@endcode

This is typically sorrounded by an excessively long compiler error
message due to failed template parameter resolutions. This cannot be avoided,
so don't panic.
Unfortunately, due to C++ compiler limitations, it is currently not possible to
give further information as to *which* data structure is missing a provider.
Therefore, check for requirements of each used provider by looking at its
`requires` list and make sure there is also a provider for that requirement.

For instance, the @ref tdc::PhiFromSA lists @ref tdc::ds::SUFFIX_ARRAY in its
requirements. Therefore, a provider that contains the suffix array in its
`provides` list needs to be given.
