#ifndef _INCLUDED_BIT_ISTREAM_HPP
#define _INCLUDED_BIT_ISTREAM_HPP

#include <climits>
#include <cstdint>
#include <iostream>

namespace tudocomp {
namespace io {

/// Wrapper over an istream that can be used to read single bits.
///
/// Read bytes are buffered on the inside until all bits of them had been read.
class BitIStream {
    std::istream& inp;
    uint8_t next = 0;
    int c;
    bool* done;

    inline void readNext() {
        const int MSB = 7;

        char tmp;
        // TODO: Error reporting
        *done |= !inp.get(tmp);
        next = tmp;

        c = MSB;
    }

public:
    /// Create a new BitIstream.
    ///
    /// \param inp_ The istream to read bits from.
    inline BitIStream(std::istream& inp_, bool& done_): inp(inp_), done(&done_) {
        c = -1;
    }

    /// Read a single bit, and return it as a byte of either the value 0 or 1.
    inline uint8_t readBit() {
        if (c < 0) {
            readNext();
        }
        uint8_t bit = (next >> c) & 0b1;
        c--;
        return bit;
    }

    /// Read a number of bits, and accumulate them in the return value
    /// with the last bit read at least significant position of the integer.
    template<class T>
    T readBits(size_t amount = sizeof(T) * CHAR_BIT) {
        T value = 0;
        for(size_t i = 0; i < amount; i++) {
            value <<= 1;
            value |= readBit();
        }
        return value;
    }

    /// Read a compressed integer from the input.
    ///
    /// \param b The block width in bits (default is 7 bits).
    template<typename T = size_t>
    inline T read_compressed_int(size_t b = 7) {
        DCHECK(b > 0);

        uint64_t value = 0;
        size_t i = 0;

        bool has_next;
        do {
            has_next = readBit();
            value |= (readBits<size_t>(b) << (b * (i++)));
        } while(has_next);

        return T(value);
    }
};

}}

#endif

