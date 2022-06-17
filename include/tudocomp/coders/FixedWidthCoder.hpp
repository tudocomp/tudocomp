
#pragma once

#include <limits>
#include <string>
#include <tudocomp/Coder.hpp>

// https://stackoverflow.com/questions/47964313/get-number-of-bits-in-size-t-at-compile-time
#if (SIZE_MAX == 0xFF)
  #define SIZE_T_BITS 8
#elif (SIZE_MAX == 0xFFFF)
  #define SIZE_T_BITS 16
#elif (SIZE_MAX == 0xFFFFFFFF)
  #define SIZE_T_BITS 32
#else
  #define SIZE_T_BITS 64
#endif

namespace tdc {

/// \brief Codes each symbol as a binary number of the specified number of bits.
///
template<size_t Width>
class FixedWidthCoder : public Algorithm {

    static_assert(Width > 0, "Cannot be encoded with width 0");
    static_assert(Width <= sizeof(size_t) * 8, "Encoding cannot be greater than the word width");

    static const size_t MASK = Width == SIZE_T_BITS ? std::numeric_limits<size_t>().max() : ~(std::numeric_limits<size_t>().max() << Width);
  public:

    /// \cond DELETED
    FixedWidthCoder() = delete;
    /// \endcond

    /// \brief Encodes data to an ASCII character stream.
    class Encoder : public tdc::Encoder {
      public:
        using tdc::Encoder::Encoder;

        template<typename value_t>
        inline void encode(value_t v, const Range &r) {
            m_out->write_int((v - r.min()) & MASK, Width);
        }
    };

    /// \brief Decodes data from an ASCII character stream.
    class Decoder : public tdc::Decoder {
      public:
        using tdc::Decoder::Decoder;

        template<typename value_t>
        inline value_t decode(const Range &r) {
            return r.min() + m_in->read_int<value_t>(Width); 
        }
    };
};


#if SIZE_T_BITS >= 8 
struct U8Coder : public FixedWidthCoder<8>{
    /// \brief Yields the coder's meta information.
    /// \sa Meta
    inline static Meta meta() {
        Meta m(Coder::type_desc(),
               "u8",
               "Encodes data using 8 bits.");
        return m;
    }
};
#endif

#if SIZE_T_BITS >= 16 
struct U16Coder : public FixedWidthCoder<16>{
    inline static Meta meta() {
        Meta m(Coder::type_desc(),
               "u16",
               "Encodes data using 16 bits.");
        return m;
    }
};
#endif

#if SIZE_T_BITS >= 32 
struct U32Coder : public FixedWidthCoder<32>{
    inline static Meta meta() {
        Meta m(Coder::type_desc(),
               "u32",
               "Encodes data using 32 bits.");
        return m;
    }
};
#endif

#if SIZE_T_BITS >= 64 
struct U64Coder : public FixedWidthCoder<64>{
    inline static Meta meta() {
        Meta m(Coder::type_desc(),
               "u64",
               "Encodes data using 64 bits.");
        return m;
    }
};
#endif

} // namespace tdc
