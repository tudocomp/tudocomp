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
    std::ostream* m_stream;
    bool m_dirty;
    uint8_t m_next;
    int m_cursor;

    inline void reset() {
        const int MSB = 7;

        m_next = 0;
        m_cursor = MSB;
        m_dirty = false;
    }

    inline void write_next() {
        if (m_dirty) {
            m_stream->put(char(m_next));
            reset();
        }
    }

public:
    /// \brief Constructs a bitwise output stream.
    ///
    /// \param output The underlying output stream.
    inline BitOStream(std::ostream& output) : m_stream(&output) {
        reset();
    }

    /// \brief Writes a single bit to the output.
    /// \param set The bit value (0 or 1).
    inline void write_bit(bool set) {
        if (set) {
            m_next |= (1 << m_cursor);
        }

        m_dirty = true;
        if (--m_cursor < 0) {
            write_next();
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
    inline void write_int(T value, size_t bits = sizeof(T) * CHAR_BIT) {
        for (int i = bits - 1; i >= 0; i--) {
            write_bit((value & T(T(1) << i)) != T(0));
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

            write_bit(v > 0);
            write_int(current, b);
        } while(v > 0);
    }

    /// \brief Forces the buffer byte to be flushed to the output. Any
    ///        unwritten bit will be unset (0).
    inline void flush() {
        write_next();
    }

    /// \brief Writes a sequence of bytes to the output.
    ///
    /// Before the sequence is written, the current buffer byte will be
    /// flushed to the output (see \c flush).
    ///
    /// \param bytes The pointer to the byte sequence to write.
    /// \param len The length of the byte sequence.
    inline void write_bytes_aligned(const char* bytes, size_t len) {
        write_next();
        m_stream->write(bytes, len);
    }

    /// \brief Provides access to the underlying output stream.
    /// \return A reference to the underlying output stream.
    inline std::ostream& stream() {
        return *m_stream;
    }
};

}}

#endif

