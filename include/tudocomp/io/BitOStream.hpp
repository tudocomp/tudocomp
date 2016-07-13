#ifndef _INCLUDED_BIT_OSTREAM_HPP
#define _INCLUDED_BIT_OSTREAM_HPP

#include <climits>
#include <cstdint>
#include <iostream>

namespace tudocomp {
namespace io {

/// Wrapper over an ostream that can be used to write single bits.
///
/// Bits are buffered in a byte until it is filled, or until flush() is called.
class BitOStream {
    std::ostream& out;
    bool dirty;
    uint8_t next;
    int c;

    inline void reset() {
        const int MSB = 7;

        this->next = 0;
        this->c = MSB;
        this->dirty = false;
    }

    inline void writeNext() {
        if (dirty) {
            out.put(char(next));
            reset();
        }
    }

public:
    /// Create a new BitOstream.
    ///
    /// \param out_ The ostream bits should be written to.
    inline BitOStream(std::ostream& out_): out(out_) {
        reset();
    }

    /// Write a single bit.
    ///
    /// \param set Wether or not the bit is set. true equals 1, false equals 0.
    inline void writeBit(bool set) {
        if (set) {
            next |= (1 << c);
        }

        dirty = true;
        if (--c < 0) {
            writeNext();
        }
    }

    /// Write a number of bits taken from a integer value, defaulting to
    /// all of them.
    ///
    /// The bits will be taken from the least significant end of the integer,
    /// but written starting with the most significant one of them.
    ///
    /// Example: Given the bits named as `76543210`, then write(_, 5) will
    /// write the bits 4, 3, 2, 1, 0 in that order.
    ///
    /// \param value The integer value to take the bits from.
    /// \param bits The number of bits to take and write.
    template<class T>
    inline void write(T value, size_t bits = sizeof(T) * CHAR_BIT) {
        for (int i = bits - 1; i >= 0; i--) {
            writeBit((value & T(T(1) << i)) != T(0));
        }
    }

    /// Write the given value as a compressed UINT.
    ///
    /// The input value is split into blocks. Each block is prepended
    /// with a bit that tells if there is a following block or not.
    /// This way, the integer is only stored in as many blocks as necessary.
    ///
    /// \param v The integer value to write.
    /// \param b The block width in bits (default is 7 bits).
    template<typename T>
    inline void write_compressed_int(T v, size_t b = 7) {
        DCHECK(b > 0);

        uint64_t u = uint64_t(v);
        uint64_t mask = (u << b) - 1;
        do {
            uint64_t current = v & mask;
            v >>= b;

            writeBit(v > 0);
            write(current, b);
        } while(v > 0);
    }

    /// If there where any bits written to the internal buffer, write
    /// them out one byte at a time, filling the gap at the end with zeroes.
    inline void flush() {
        writeNext();
    }

    /// Write a number of bytes aligned to the byte grid.
    ///
    /// If the internal buffer currently holds a number of bits less than 8,
    /// the remainder will be filled with zeroes before writing the bytes.
    inline void write_aligned_bytes(const char* bytes, size_t len) {
        writeNext();
        out.write(bytes, len);
    }

    inline std::ostream& stream() {
        return out;
    }
};

}}

#endif

