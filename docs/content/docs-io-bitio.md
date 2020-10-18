@page bitio Bitwise I/O

# Bitwise I/O
Bitwise input and output is provided by the classes @ref tdc::BitIStream and
@ref tdc::BitOStream.  They are wrappers around `std::istream` and
`std::ostream`, respectively, and provide functionality to read or write single
bits or fixed-width integers (in MSBF order) from/to their underlying
stream.

Single bits are written using @ref tdc::BitOStream::write_bit, integers in
binary representation using @ref tdc::BitOStream::write_int. The following
example performs several bitwise write operations on an output:

@code{.cpp}
{
BitOStream obits(output); //construct the bitwise output stream

obits.write_bit(0);     // write a single unset bit
obits.write_bit(1);     // write a single set bit
obits.write_int(27, 5); // write the value 27 using 5 bits (11011)
obits.write_int(27, 3); // write the value 27 using 3 bits (truncated to 011)

int a = 27;
obits.write_int(a); // write the value 27 using 8*sizeof(int) bits (32)
                    // (00000000000000000000000000011011)

uint8_t b = 27;
obits.write_int(b); // write the value 27 using 8*sizeof(uint8_t) bits (8)
                    // (00011011)

} // end of scope, write EOF sequence and destroy the bit output stream
@endcode

There is important logic behind BitOStream's destructor: since the stream
writes bits to an underlying byte stream, it needs to write a few extra bits at
the end of the stream in order to indicate the end for a bit input stream.

Note how `write_int` will use the default size of the passed integer's type if
no bit width is explicitly passed in the second argument.

The following example performs several bitwise read operations from an input:

@code{.cpp}
BitIStream ibits(input); // construct the bitwise input stream

bool bit = ibits.read_bit(); // read a single bit

uint8_t  a = ibits.read_int<uint8_t>(5); // read a 5-bit integer into a uint8_t
uint16_t b = ibits.read_int<uint16_t>(); // read a 16-bit integer
@endcode

Note how `read_int` requires a template parameter telling it into which data
type the read integer will be stored and perform the respective conversion.
If no bit width is given, the default size of the data type will be used.

#### Universal codes.

Beyond writing single bits and binary-coded integers, the following universal
integer codes are available:

* Unary code (@ref tdc::BitOStream::write_unary /
  @ref tdc::BitIStream::read_unary)
* Ternary code (@ref tdc::BitOStream::write_ternary /
  @ref tdc::BitIStream::read_ternary)
* Elias Gamma code (@ref tdc::BitOStream::write_elias_gamma /
  @ref tdc::BitIStream::read_elias_gamma)
* Elias Delta code (@ref tdc::BitOStream::write_elias_delta /
  @ref tdc::BitIStream::read_elias_delta)
* Rice code (@ref tdc::BitOStream::write_rice /
  @ref tdc::BitIStream::read_rice)
* VByte coding (@ref tdc::BitOStream::write_compressed_int /
  @ref tdc::BitIStream::read_compressed_int)
