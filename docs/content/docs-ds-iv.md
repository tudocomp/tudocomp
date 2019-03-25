@page iv Arbitrary-Width Integer Vectors

# Arbitrary-Width Integer Vectors

tudocomp provides types and classes to allow the use of arbitrary-width
integers and vectors thereof. These are not limited to a default widths of 8,
16, 32 or 64 bits, but can be stored using any amount of bits as necessary.

The template class @ref uint_t is used to store single arbitrary-width integers.
For instance, the specialization `uint_t<40>` can be
used to represent values up to 2^40^-1. `uint_t` typically behaves like any
other unsigned integer type, providing the respective operators and automatic
casts.

The true power of arbitrary-width integers feature is unleashed by using the
@ref tdc::IntVector class to store sequences of them. Instead of storing values
in a byte-aligned manner like standard vectors, IntVector stores them as a
bitwise sequence of binary representations. This can dramatically reduce the
size of vectors when the optimal bit width required for storing a single is not
a power of two.

IntVector can be used with static or dynamic bit widths as explained in
the following sections.

## Static-width integer vectors

The bit width of values in static-width integer vectors is fixed at compile
time. This is the standard behaviour if, for example, `IntVector<uint_t<40>>`
is used for vectors of 40-bit integers.

A notable use case for this is the @ref tdc::BitVector specialization,
an alias for `IntVector<uint_t<1>>`, which effectively implements bit vectors.

The following example illustrates the use of IntVector:

@code{.cpp}
// reserve a vector of 32 4-bit integers (initialized as zero)
IntVector<uint_t<4>> iv4(32);

// fill it with increasing values (0, 1, 2, ...)
std::iota(iv4.begin(), iv4.end(), 0);

// print size in bits
LOG(INFO) << "Size of iv4 in bits: " << iv4.bit_size();

// demonstrate overflow
ASSERT_EQ(iv4[0], iv4[16]);

// reserve an additional bit vector with 32 entries
BitVector bv(32);

// mark all multiples of 3
for(len_t i = 0; i < 32; i++) {
    bv[i] = ((iv4[i] % 3) == 0);
}
@endcode

This example also demonstrates how `IntVector` is implemented in a fully
STL-compatible way.

Note how arbitrary-width integers overflow in the expected fashion: `iv4[16]`
-- which should be 16 according to
[`std::iota`](http://en.cppreference.com/w/cpp/algorithm/iota)) --
equals `iv4[0]`, which is zero, because the value 16 cannot be represented using
only four bits.

### Dynamic-width integer vectors

Using the specialization @ref tdc::DynamicIntVector (which is an alias for
`IntVector<dynamic_t>`), the bit width of values can be specified at runtime.

This is useful when the required width depends on previously processed input
(e.g., a text length). It can also be used to bit-compress an integer vector
after its initial construction.

The following is an example for the usage of DynamicIntVector:

@code{.cpp}
// reserve a vector for 20 integer values (initialized as zero)
// default to a width of 32 bits per value
DynamicIntVector fib(20, 0, 32);

// fill it with the Fibonacci sequence
fib[0] = 0;
fib[1] = 1;
for(len_t i = 2; i < fib.size(); i++) {
  fib[i] = fib[i - 2] + fib[i - 1];
}

// find the amount of bits required to store the last (and largest) value
auto max_bits = bits_for(fib.back());

// bit-compress the vector to the amount of bits required
fib.width(max_bits);
fib.shrink_to_fit();
@endcode

Initially, the `fib` vector contains 20 integers of width 32 bits each, i.e.,
640 bits in total. After bit compression, the vector will have a bit width
of 13 (which is the amount of bits required to store 6,765, the 20^th^
Fibonacci number) and thus requires only 320 bits (260 bits for the data, 60
bits of overhead for 64-bit-alignment, as the vector is backed by an array
of 64-bit integers).

The @ref tdc::bits_for function is used to determine the
amount of bits required to store an integer by computing the rounded-up binary
logarithm. In the constructor and using @ref tdc::DynamicIntVector::width,
the bit width of values within the vector can be altered at runtime.
In order to achieve bit-compression, a call to
@ref tdc::DynamicIntVector::shrink_to_fit is necessary in order to actually
shrink the vector's capacity.