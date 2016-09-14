#ifndef _INCLUDED_BIT_ISTREAM_HPP
#define _INCLUDED_BIT_ISTREAM_HPP

#include <climits>
#include <cstdint>
#include <iostream>

namespace tudocomp {
namespace io {

/// \brief Wrapper for input streams that provides bitwise reading
/// functionality.
///
/// The current byte in the underlying input stream is buffered and processed
/// bitwise using another cursor.
class BitIStream {
    std::istream* m_stream;
    uint8_t m_next = 0;
    int m_cursor;
    bool* m_done;

    inline void read_next() {
        const int MSB = 7;

        char tmp;
        // TODO: Error reporting
        *m_done |= !m_stream->get(tmp);
        m_next = tmp;

        m_cursor = MSB;
    }

public:
    /// \brief Constructs a bitwise input stream.
    ///
    /// \param input The underlying input stream.
    /// \param done A reference to a flag that is set to \c true when the
    ///              underlying input stream has been read completely.
    inline BitIStream(std::istream& input, bool& done)
        : m_stream(&input), m_done(&done) {
        m_cursor = -1;
    }

    /// \brief Reads the next single bit from the input.
    /// \return 1 if the next bit is set, 0 otherwise.
    inline uint8_t read_bit() {
        if (m_cursor < 0) {
            read_next();
        }
        uint8_t bit = (m_next >> m_cursor) & 1;
        m_cursor--;
        return bit;
    }

    /// \brief Reads the integer value of the next \c amount bits in MSB first
    ///        order.
    /// \tparam The integer type to read.
    /// \param amount The bit width of the integer to read. By default, this
    ///               equals the bit width of type \c T.
    /// \return The integer value of the next \c amount bits in MSB first
    ///         order.
    template<class T>
    T read_int(size_t amount = sizeof(T) * CHAR_BIT) {
        T value = 0;
        for(size_t i = 0; i < amount; i++) {
            value <<= 1;
            value |= read_bit();
        }
        return value;
    }

    /// \brief Reads a compressed integer from the input.
    ///
    /// The \e compressed form of an integer \c n is achieved by splitting
    /// up the bit representation of \c n in blocks of width \c b. For each
    /// non-zero block (in little endian order), a 1-bit and the block itself
    /// is written. The compressed integer is finally terminated by a 0-bit.
    ///
    /// \tparam The integer type to read.
    /// \param b The block width in bits. The default is 7 bits.
    /// \return The read integer value.
    template<typename T = size_t>
    inline T read_compressed_int(size_t b = 7) {
        DCHECK(b > 0);

        uint64_t value = 0;
        size_t i = 0;

        bool has_next;
        do {
            has_next = read_bit();
            value |= (read_int<size_t>(b) << (b * (i++)));
        } while(has_next);

        return T(value);
    }

    /// \brief Provides access to the underlying input stream.
    /// \return A reference to the underlying input stream.
    inline std::istream& stream() {
        return *m_stream;
    }
};

}}

#endif

