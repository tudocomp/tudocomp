#ifndef _INCLUDED_BIT_OSTREAM_HPP
#define _INCLUDED_BIT_OSTREAM_HPP

#include <climits>
#include <cstdint>
#include <iostream>

namespace tudocomp {
namespace io {

/// \brief Wrapper for output streams that provides bitwise writing
/// functionality.
///
/// Bits are written into a buffer byte, which is written to the output when
/// it is either filled or when a flush is explicitly requested.
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
    /// \brief Constructs a bitwise output stream.
    ///
    /// \param out_ The underlying output stream.
    inline BitOStream(std::ostream& out_): out(out_) {
        reset();
    }

    /// \brief Writes a single bit to the output.
    /// \param set The bit value (0 or 1).
    inline void writeBit(bool set) {
        if (set) {
            next |= (1 << c);
        }

        dirty = true;
        if (--c < 0) {
            writeNext();
        }
    }

    /// Writes the bit representation of an integer in MSB first order to
    /// the output.
    ///
    /// \tparam The type of integer to write.
    /// \param value The integer to write.
    /// \param bits The amount of low bits of the value to write. By default,
    ///             this equals the bit width of type \c T.
    template<class T>
    inline void write(T value, size_t bits = sizeof(T) * CHAR_BIT) {
        for (int i = bits - 1; i >= 0; i--) {
            writeBit((value & T(T(1) << i)) != T(0));
        }
    }

    /// \brief Writes a compressed integer to the input.
    ///
    /// The \e compressed form of an integer \c n is achieved by splitting
    /// up the bit representation of \c n in blocks of width \c b. For each
    /// non-zero block (in little endian order), a 1-bit and the block itself
    /// is written. The compressed integer is finally terminated by a 0-bit.
    ///
    /// \tparam The integer type to write.
    /// \param v The integer to write.
    /// \param b The block width in bits. The default is 7 bits.
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

    /// \brief Forces the buffer byte to be flushed to the output. Any
    ///        unwritten bit will be unset (0).
    inline void flush() {
        writeNext();
    }

    /// \brief Writes a sequence of bytes to the output.
    ///
    /// Before the sequence is written, the current buffer byte will be
    /// flushed to the output (see \c flush).
    ///
    /// \param bytes The pointer to the byte sequence to write.
    /// \param len The length of the byte sequence.
    inline void write_aligned_bytes(const char* bytes, size_t len) {
        writeNext();
        out.write(bytes, len);
    }

    /// \brief Provides access to the underlying output stream.
    /// \return A reference to the underlying output stream.
    inline std::ostream& stream() {
        return out;
    }
};

}}

#endif

