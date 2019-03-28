@page types Type Definitions

# Type Definitions
tudocomp defines several types that we recommend to use in certain
scenarios.

## Lengths and Array Indices
Many algorithms require storage of indices pointing to certain positions
in the input. For example, the suffix array is an array of *n*
indices into the text of length *n*. The use of standard unsigned
integer types (e.g., `std::size_t`), this bears a disadvantage:
when storing a large number of array indices, a lot of memory is wasted
for alignment, since `std::size_t` is typically a 64-bit type, but
inputs are unlikely to be a size requiring 64-bit addressing.

We recommend using the @ref tdc::len_compact_t type for this use case.
Its size is determined by the `LEN_BITS` build configuration
parameter (see below) and defaults to 32 bits. For handling inputs
up to one terabyte, it can be set to 40 bits, for example. This way,
the maximum input size that the code can handle can be configured in
the build without the need to change any code. `len_compact_t` is an alias
for @ref tdc::uint_t<LEN_BITS> and thus can be used for our
@ref tdc::IntVector data structure to create sparse arrays. The
following is a brief comparison of using `size_t` versus
`len_compact_t`.

@code{.cpp}
// std::size_t
size_t size_array[n]; // uses n * 64 bits of memory on 64-bit architectures

// using len_compact_t
len_compact_t index_array[n]; // uses n * ceil(LEN_BITS/8) bits of memory

// sparse
IntVector<len_compact_t> sparse_array[n]; // uses roughly n * LEN_BITS bits of memory
@endcode

#### Configuring the bit width.
The bit width of `len_compact_t` can be determined by defining the
`LEN_BITS` variable for the CMake build script. For more information
see @ref build.

#### Fast alternative for computations.
The fact that @ref tdc::uint_t -- and thus `len_compact_t` --
may be of any bit width, causes overhead when using it for arithmetic
operations. If computations need to be done with index values, we
recommend using @ref tdc::len_t, which is an alias for the next smallest
standard unsigned integer type that can fit a `len_compact_t` (e.g.,
if `LEN_BITS` is 40, `len-t` is defined as `uint64_t`).

## Text Characters
For single text characters (literals), tudocomp defines
@ref tdc::uliteral_t, which we recommend using over `char`. It is an
alias for `unsigned char` by default, with the idea that it can be
substituted by other types if needed.
