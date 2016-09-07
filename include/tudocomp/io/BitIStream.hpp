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
    /// \brief Constructs a bitwise input stream.
    ///
    /// \param inp_ The underlying input stream.
    /// \param done_ A reference to a flag that is set to \c true when the
    ///              underlying input stream has been read completely.
    inline BitIStream(std::istream& inp_, bool& done_): inp(inp_), done(&done_) {
        c = -1;
    }

    /// \brief Reads the next single bit from the input.
    /// \return 1 if the next bit is set, 0 otherwise.
    inline uint8_t readBit() {
        if (c < 0) {
            readNext();
        }
        uint8_t bit = (next >> c) & 0b1;
        c--;
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
    T readBits(size_t amount = sizeof(T) * CHAR_BIT) {
        T value = 0;
        for(size_t i = 0; i < amount; i++) {
            value <<= 1;
            value |= readBit();
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
            has_next = readBit();
            value |= (readBits<size_t>(b) << (b * (i++)));
        } while(has_next);

        return T(value);
    }
};

}}

#endif

