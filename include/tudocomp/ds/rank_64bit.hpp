#pragma once

#include <cstdint>
#include <tudocomp/util.hpp>

namespace tdc {

    /// \brief Lookup table for the rank operation on 8-bit values.
    ///
    /// At index \c i, this table contains the amount of 1-bits in the binary
    /// representation of the value \c i.
    constexpr uint8_t rank1_8bit[] = {
        0, 1, 1, 2, 1, 2, 2, 3,
        1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4,
        2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4,
        2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5,
        3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4,
        2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5,
        3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5,
        3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6,
        4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4,
        2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5,
        3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5,
        3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6,
        4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5,
        3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6,
        4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6,
        4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7,
        5, 6, 6, 7, 6, 7, 7, 8
    };

    /// \brief Computes the amount of 1-bits in the binary representation of
    ///        the given 8-bit value.
    /// \param v the value in question
    /// \return the amount of 1-bits in the value's binary representation
    inline constexpr uint8_t rank1(uint8_t v) {
        return rank1_8bit[v];
    }

    /// \brief Computes the amount of 1-bits in the binary representation of
    ///        the given 16-bit value.
    /// \param v the value in question
    /// \return the amount of 1-bits in the value's binary representation
    inline constexpr uint8_t rank1(uint16_t v) {
        return __builtin_popcount(v);
    }

    /// \brief Computes the amount of 1-bits in the binary representation of
    ///        the given 32-bit value.
    /// \param v the value in question
    /// \return the amount of 1-bits in the value's binary representation
    inline constexpr uint8_t rank1(uint32_t v) {
        return __builtin_popcount(v);
    }

    /// \brief Computes the amount of 1-bits in the binary representation of
    ///        the given 64-bit value.
    /// \param v the value in question
    /// \return the amount of 1-bits in the value's binary representation
    inline constexpr uint8_t rank1(uint64_t v) {
        return __builtin_popcountll(v);
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given value.
    ///
    /// The rank value computed is that of the bit interval [0,m] in
    /// zero-based LSBF order.
    ///
    /// \tparam uint_t the type of the value in question
    /// \param v the value in question
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    template<typename uint_t>
    inline constexpr uint8_t rank1(uint_t v, uint8_t m) {
        DCHECK(m <= msbf<uint_t>::pos) << "m=" << m;
        const uint_t mask =
            std::numeric_limits<uint_t>::max() >> (msbf<uint_t>::pos-m);
        return rank1(uint_t(v & mask));
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given value.
    ///
    /// The rank value computed is that of the bit interval [l,m] in
    /// zero-based LSBF order.
    ///
    /// \tparam uint_t the type of the value in question
    /// \param v the value in question
    /// \param l the least significant bit (from which the counting starts)
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    template<typename uint_t>
    inline constexpr uint8_t rank1(uint_t v, uint8_t l, uint8_t m) {
        DCHECK(l <= msbf<uint_t>::pos &&
               m <= msbf<uint_t>::pos &&
               l <= m) << "l=" << l << ",m=" << m;

        const uint_t mask_m =
            std::numeric_limits<uint_t>::max() >> (msbf<uint_t>::pos-m);
        const uint_t mask_l =
            std::numeric_limits<uint_t>::max() << l;
        return rank1(uint_t(v & mask_m & mask_l));
    }

    template<typename uint_t>
    inline constexpr uint8_t rank0(uint_t v) {
        return msbf<uint_t>::pos + 1 - rank1(v);
    }

    template<typename uint_t>
    inline constexpr uint8_t rank0(uint_t v, uint8_t m) {
        return (m + 1) - rank1(v, m);
    }

    template<typename uint_t>
    inline constexpr uint8_t rank0(uint_t v, uint8_t l, uint8_t m) {
        return (m - l + 1) - rank1(v, l, m);
    }

} //ns
