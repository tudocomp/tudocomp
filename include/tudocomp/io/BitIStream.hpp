#pragma once

#include <climits>
#include <cstdint>
#include <iostream>
#include <tudocomp/util.hpp>

namespace tdc {
namespace io {

/// \brief Wrapper for input streams that provides bitwise reading
/// functionality.
///
/// The current byte in the underlying input stream is buffered and processed
/// bitwise using another cursor.
class BitIStream {
    InputStream m_stream;

    uint8_t m_current, m_next;

    bool m_is_final;
    uint8_t m_final_bits;

    uint8_t m_cursor;

    inline void read_next() {
        const uint8_t MSB = 7;

        m_current = m_next;
        m_cursor = MSB;

        char c;
        if(m_stream.get(c)) {
            m_next = c;

            if(m_stream.get(c)) {
                /*DLOG(INFO) << "read_next: ...";*/

                // stream still going
                m_stream.unget();
            } else {
                // stream over after next, do some checks
                m_final_bits = c;
                m_final_bits &= 0x7;
                if(m_final_bits >= 6) {
                    // special case - already final
                    m_is_final = true;
                    m_next = 0;
                    /*DLOG(INFO) << "read_next: EOF*" <<
                        ", m_final_bits := " << size_t(m_final_bits);*/
                }
            }
        } else {
            m_is_final = true;
            m_final_bits = m_current & 0x7;

            m_next = 0;

            /*DLOG(INFO) << "read_next: EOF" <<
                ", m_final_bits := " << size_t(m_final_bits);*/
        }
    }

public:
    /// \brief Constructs a bitwise input stream.
    ///
    /// \param input The underlying input stream.
    inline BitIStream(InputStream&& input) : m_stream(std::move(input)) {
        char c;
        if(m_stream.get(c)) {
            m_is_final = false;
            m_next = c;

            read_next();
        } else {
            //empty stream
            m_is_final = true;
            m_final_bits = 0;
        }
    }

    /// \brief Constructs a bitwise input stream.
    ///
    /// \param input The underlying input.
    inline BitIStream(Input& input) : BitIStream(input.as_stream()) {
    }

    /// \brief Reads the next single bit from the input.
    /// \return 1 if the next bit is set, 0 otherwise.
    inline uint8_t read_bit() {
        /*DLOG(INFO) <<"read_bit: " <<
                "m_is_final = " << m_is_final <<
                ", m_final_bits = " << size_t(m_final_bits) <<
                ", m_cursor = " << size_t(m_cursor);*/

        if(!eof()) {
            uint8_t bit = (m_current >> m_cursor) & 1;
            if(m_cursor) {
                --m_cursor;
            } else {
                read_next();
            }

            return bit;
        } else {
            return 0; //EOF
        }
    }

    /// \brief Reads the integer value of the next \c amount bits in MSB first
    ///        order.
    /// \tparam The integer type to read.
    /// \param amount The bit width of the integer to read. By default, this
    ///               equals the bit width of type \c T.
    /// \return The integer value of the next \c amount bits in MSB first
    ///         order.
    template<class T>
    inline T read_int(size_t amount = sizeof(T) * CHAR_BIT) {
        T value = 0;
        for(size_t i = 0; i < amount; i++) {
            value <<= 1;
            value |= read_bit();
        }
        return value;
    }

    template<typename value_t>
    inline value_t read_unary() {
        value_t v = 0;
        while(!read_bit()) ++v;
        return v;
    }

    template<typename value_t>
    inline value_t read_ternary() {
        size_t mod = read_int<size_t>(2);
        value_t v = 0;
        if(mod < 3) {
            size_t b = 1;
            do {
                v += mod * b;
                b *= 3;
                mod = read_int<size_t>(2);
            } while(mod != 3);

            ++v;
        }
        return v;
    }

    template<typename value_t>
    inline value_t read_elias_gamma() {
        auto bits = read_unary<size_t>();
        return read_int<value_t>(bits);
    }

    template<typename value_t>
    inline value_t read_elias_delta() {
        auto bits = read_elias_gamma<size_t>();
        return read_int<value_t>(bits);
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

    /// TODO document
    inline bool eof() const {
        return m_is_final && m_cursor <= (7 - m_final_bits);
    }
};

}}

