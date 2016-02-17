#ifndef BIT_IOSTREAM_H
#define BIT_IOSTREAM_H

#include "glog/logging.h"

#include <cstdint>
#include <iostream>
#include <climits>

namespace tudocomp {

/// Wrapper over an ostream that can be used to write single bits.
///
/// Bits are buffered in a byte until it is filled, or until flush() is called.
class BitOstream {
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
    inline BitOstream(std::ostream& out_): out(out_) {
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
};

/// Wrapper over an istream that can be used to read single bits.
///
/// Read bytes are buffered on the inside until all bits of them had been read.
class BitIstream {
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
    inline BitIstream(std::istream& inp_, bool& done_): inp(inp_), done(&done_) {
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
};

}

#endif
