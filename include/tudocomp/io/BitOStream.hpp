#pragma once

#include <climits>
#include <cstdint>
#include <iostream>
#include <tudocomp/util.hpp>
#include <tudocomp/io/Output.hpp>

namespace tdc {
namespace io {

/// \brief Wrapper for output streams that provides bitwise writing
/// functionality.
///
/// Bits are written into a buffer byte, which is written to the output when
/// it is either filled or when a flush is explicitly requested.
class BitOStream {
    OutputStream m_stream;

    uint8_t m_next;
    int8_t m_cursor;
    static constexpr int8_t MSB = 7;

    inline bool is_dirty() const {
        return m_cursor != MSB;
    }

    inline void reset() {
        m_next = 0;
        m_cursor = MSB;
    }

    inline void write_next() {
        m_stream.put(char(m_next));
        reset();
    }

public:
    /// \brief Constructs a bitwise output stream.
    ///
    /// \param output The underlying output stream.
    inline BitOStream(OutputStream&& output) : m_stream(std::move(output)) {
        reset();
    }

    /// \brief Constructs a bitwise output stream.
    ///
    /// \param output The underlying output.
    inline BitOStream(Output& output) : BitOStream(output.as_stream()) {
    }

    BitOStream(BitOStream&& other) = default;

    inline ~BitOStream() {
        uint8_t set_bits = MSB - m_cursor; // will only be in range 0 to 7
        if(m_cursor >= 2) {
            // if there are at least 3 bits free in the byte buffer,
            // write them into the cursor at the last 3 bit positions
            m_next |= set_bits;
            write_next();
        } else {
            // else write out the byte, and write the length into the
            // last 3 bit positions of the next byte
            write_next();
            m_next = set_bits;
            write_next();
        }
    }

    /// \brief Writes a single bit to the output.
    /// \param set The bit value (0 or 1).
    inline void write_bit(bool set) {
        if (set) {
            m_next |= (1 << m_cursor);
        }

        m_cursor--;

        if (m_cursor < 0) {
            write_next();
        }
    }

    /// \brief Asserts that the next write operation starts on a byte boundary
    inline void assert_byte_boundary() const {
        CHECK(!is_dirty());
    }

    /// \brief Returns the underlying stream.
    ///
    /// Note that the stream does not include bits that have not yet been
    /// flushed.
    inline std::ostream& stream() {
        return m_stream;
    }

    // Only higher level functions that use write_bit below:

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

    template<typename value_t>
    inline void write_unary(value_t v) {
        while(v--) {
            write_bit(0);
        }

        write_bit(1);
    }

    template<typename value_t>
    inline void write_ternary(value_t v) {
        if(v) {
            --v;
            do {
                write_int(v % 3, 2); // 0 -> 00, 1 -> 01, 2 -> 10
                v /= 3;
            } while(v);
        }
        write_int(3, 2); // terminator -> 11
    }

    template<typename value_t>
    inline void write_elias_gamma(value_t v) {
        write_unary(bits_for(v));
        write_int(v, bits_for(v));
    }

    template<typename value_t>
    inline void write_elias_delta(value_t v) {
        write_elias_gamma(bits_for(v));
        write_int(v, bits_for(v));
    }

    /// \brief Writes a compressed integer to the input.
    ///
    /// The \e compressed form of an integer \c n is achieved by splitting
    /// up the bit representation of \c n in blocks of width \c b. For each
    /// non-zero block (in little endian order), a bit flag and the block itself
    /// is written.
    ///
    /// The flag is set to 1 if there is another block after the current one,
    /// and otherwise set to 0 to mark the block as the last one.
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
};

}}

