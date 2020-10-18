@page coder Coders

# Coders
Coders serve for encoding primitve data types into a bit sequence with limited
context. This is contrary to compressors, which may have random access the
entire input sequence. The only context available to coders consists of
*value ranges* and information on the *input alphabet*.

While compressors should provide as much context information as possible
to coders, it is up to the coder to actually make use of it or only parts of it.

### Ranges

A Range (@ref tdc::Range) serves as a representation of a value range, i.e.,
a minimum and a maximum value. Having information on the range that a certain
value is contained in can help to encode the value in an efficient manner
using less bits. For example, if a value *x* should be encoded and it is known
that *1024 <= x < 1088* holds, the coder could make use of this information
in various ways:

* Encode only the difference *d~x~ = x - 1024*.
* Encode the difference using only 6 bits (because *d~x~ < 64* holds).
* If it is also known that *x* tends to be distributed towards the minimum
  value, use variable-width encoding for *d~x~*.
* ...

tudocomp provides several classes of ranges that should be used as precisely
as possible in order to provide the best possible context information to coders:

| Class | Description |
|-------|-------------|
| @ref tdc::Range               | A simple min-max-range with no particular distribution. |
| @ref tdc::MinDistributedRange | Values tend to be distributed towards the minimum.      |
| @ref tdc::BitRange            | Values are either 0 or 1.                               |
| @ref tdc::TypeRange<T>        | Values are of a certain integer type `T`.               |
| @ref tdc::LiteralRange        | Values are literals (of type `uliteral_t`, see below).  |

For literals, that is, characters of the input alphabet (commonly represented
using the @ref tdc::uliteral_t type), `LiteralRange` serves a special purpose
and should be favored. The following section on literal iterators will
provide more information.

The following ranges are predefined globally:
* @ref tdc::bit_r (pre-allocated `BitRange`)
* @ref tdc::uliteral_r (pre-allocated `LiteralRange`)
* @ref tdc::len_r (pre-allocated `TypeRange<len_compact_t>`)
* @ref tdc::len_r (pre-allocated `TypeRange<size_t>`)

### Literal Iterators
Another useful piece of context information for coders is the input alphabet,
that is, the characters or bytes (henceforth called "literals") that
occur in the input and where and how often they occur. This information can be
used for low-entropy coding, a prominent example being Huffman codes. In order
to provide coders with information on the input literals, tudocomp uses the
concept of *literal iterators* (@ref tdc::LiteralIterator). A literal iterator
must define the functions `has_next` and `next`, which the coder uses to
navigate over the inpu literals. Information on a literal provided by `next`
comes in @ref tdc::Literal objects, which contain the literal as well as the
position at which it occurs.

The most simple form of a literal iterator would report every literal from the
input in sequential order along with their position. For example, for the input
text `"aba"`, it would yield the following Literal objects (represented as
tuples of literal and position of occurence): `(a,1)`, `(b,2)`, `(a,3)`. This
most simple behavior is implemented in the @ref tdc::ViewLiterals class.

However, there are situations where this does not correspond to the literal
occurences of an already processed input. A simple example is the LZ77
factorization of a text: Some literals have been replaced by references
and would, if still counted, distort the distribution of literals for a Huffman
encoding. Therefore, the literal iterator of a LZ77 compressor should skip any
literal that has been replaced by a reference. This commonly results in more
complex implementations of the aforementioned functions `has_next` and `next`.

For some scenarios, especially during development, the @ref tdc::NoLiterals
iterator can be used to give no input alphabet information to the coder at all.
Note that low-entropy encoders that rely on this information (such as Huffman)
are not compatible with this.

### The Coder Interface
Coders are logically split into *encoders* (@ref tdc::Encoder) and *decoders*
(@ref tdc::Decoder). It is expected that for any encoder, there is a
corresponding decoder that restores the original output. In order to enforce
this, encoder and decoder implementations are typically encapsulated in an outer
`Coder` class that inherit from `Algorithm`. This design pattern allows for
separation of encoding and decoding functionality while retaining the ability
to instantiate both from a template type (ie. the outer class).

Apart from this concept, encoders and decoders are simple classes that declare
the function templates @ref tdc::Encoder::encode and @ref tdc::Decoder::decode,
respectively. Their template parameters are generally expected to be numeric
types. Both `encode` and `decode` accept a range as an argument. As discussed
above, the range information can be used to encode a given value in a more
efficient manner. The following example uses the range in order to perform a
binary encoding of the given value:

@code{.cpp}
template<typename value_t>
inline void encode(value_t v, const Range& r) {
    // Compute the amount of bits required to store a value of range r
    auto delta_bits = bits_for(r.delta());

    // Encode the value as binary using the computed amount of bits
    v -= r.min();
    m_out->write_int(v, delta_bits);
}
@endcode

@ref tdc::Range::delta is a convenience function that returns the difference
between the range's maximum and minimum. In the example, the number of bits
required to store `delta` in binary representation is computed using 
@ref tdc::bits_for. Then, only the difference between the passed value
and the range's minimum value is actually encoded. The corresponding decoding
function could be implemented in the following way:

@code{.cpp}
template<typename value_t>
inline value_t decode(const Range& r) {
    // Compute the amount of bits required to store a value of range r
    auto delta_bits = bits_for(r.delta());

    // Decode the value from binary reading the computed amount of bits
    value_t v = m_in->read_int<value_t>(delta_bits);
    v += r.min();
    return v;
}
@endcode

Again, the amount of bits for the range's delta is computed and the difference
of the stored value and the minimum is decoded. In a final step, naturally,
the range's minimum has to be added back in order to restore the original value.

Note that the decoder relies on its controller (e.g. a compressor) to provide
the correct range information. In order for the above example to work, the same
range has to be used to encode and decode each value.

#### Range overloads.

In order to implement different behavior for different types of ranges,
we use overloads of `encode` and `decode`. These overloads only differ to the
default signature in that they accept more specialized Range objects. The
following example realizes an ASCII encoding for single bits, as identified by
@ref tdc::BitRange.

@code{.cpp}
template<typename value_t>
inline void encode(value_t v, const BitRange&) {
    // Encode single bits as ASCII
    m_out->write_int(v ? '1' : '0');
}
@endcode

The same works for decoding:
@code{.cpp}
template<typename value_t>
inline value_t decode(const BitRange&) {
    // Decode an ASCII character and compare against '0'
    uint8_t b = m_in->read_int<uint8_t>();
    return (b != '0');
}
@endcode

Note that there is *no* virtual inheritance, and therefore no polymorphism,
between the different range types. This may be a pitfall when trying to
introduce new range types. Also note that the default Encoder and Decoder
implementations provide overloads for the default range (which do binary
coding like above) as well as for BitRange (which writes single bits).

#### Using Literal Iterators
Encoders accept a literal iterator via their constructor. The following simple
example uses the literal iterator to count the amount of occurences of each
possible literal (0 to 255) in the input, i.e., create a histogram:

@code{.cpp}
// count occurences of each literal
len_t occ[ULITERAL_MAX + 1];

while(literals.has_next()) {
    Literal lit = literals.next();
    ++occ[lit.c];
}
@endcode

## Interleaved Coding
In order to achieve better (smaller) results, different encoders could be used
for different data payloads. For example, when encoding a LZ78 factorization,
one could use binary coding for a factor ID and Huffman coding for the appended
literal. This is called *interleaved* coding, since both encoders would write to
the same bit output stream. Issues with interleaved coding arise when coders
"absorb" a certain number of values before producing an output -- for example
run-length encoders that first count the length of a run, or arithmetic coders
that compute a code step by step. In case a different encoder writes values in
the meantime, the order of written items may be corrupted.

To prevent this, encoders need to implement the @ref tdc::Encoder::flush
function. Calling it ensures that whatever is currently cached will be encoded
and it is safe to use a different coder on the output.

> TODO: example

## Available Coders

Out of the box, tudocomp currently implements several coders, including:

* Human readable coding (ASCII, for debugging purposes)
* Binary coding
* Universal codes (e.g. Elias and Rice codes)
* Statistic codes (e.g. Huffman code)

A full list can be found in the inheritance diagram of the API reference
for @ref tdc::Encoder.
