#pragma once

#include <cstdint>

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
        return rank1_8bit[uint8_t(v >> 8)] + rank1_8bit[uint8_t(v)];
    }

    /// \brief Computes the amount of 1-bits in the binary representation of
    ///        the given 32-bit value.
    /// \param v the value in question
    /// \return the amount of 1-bits in the value's binary representation
    inline constexpr uint8_t rank1(uint32_t v) {
        return rank1_8bit[uint8_t(v >> 24)] + rank1_8bit[uint8_t(v >> 16)] +
               rank1_8bit[uint8_t(v >>  8)] + rank1_8bit[uint8_t(v)];
    }

    /// \brief Computes the amount of 1-bits in the binary representation of
    ///        the given 64-bit value.
    /// \param v the value in question
    /// \return the amount of 1-bits in the value's binary representation
    inline constexpr uint8_t rank1(uint64_t v) {
        return rank1_8bit[uint8_t(v >> 56)] + rank1_8bit[uint8_t(v >> 48)] +
               rank1_8bit[uint8_t(v >> 40)] + rank1_8bit[uint8_t(v >> 32)] +
               rank1_8bit[uint8_t(v >> 24)] + rank1_8bit[uint8_t(v >> 16)] +
               rank1_8bit[uint8_t(v >>  8)] + rank1_8bit[uint8_t(v)];
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given 8-bit value.
    ///
    /// The exact rank value computed is that of the interval [0,m] in
    /// zero-based LSBF order.
    ///
    /// \param v the value in question
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    inline constexpr uint8_t rank1(uint8_t v, uint8_t m) {
        DCHECK(m < 8) << "m=" << m;
        const uint8_t mask = 0xFFU >> (7-m);
        return rank1(uint8_t(v & mask));
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given 8-bit value.
    ///
    /// The exact rank value computed is that of the interval [l,m] in
    /// zero-based LSBF order.
    ///
    /// \param v the value in question
    /// \param l the least significant bit (from which the counting starts)
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    inline constexpr uint8_t rank1(uint8_t v, uint8_t l, uint8_t m) {
        DCHECK(l < 8 && m < 8 && l <= m) << "l=" << l << ",m=" << m;
        const uint8_t mask_m = UINT8_MAX >> (7-m);
        const uint8_t mask_l = UINT8_MAX << l;
        return rank1(uint8_t(v & mask_m & mask_l));
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given 16-bit value.
    ///
    /// The exact rank value computed is that of the interval [0,m] in
    /// zero-based LSBF order.
    ///
    /// \param v the value in question
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    inline constexpr uint8_t rank1(uint16_t v, uint8_t m) {
        DCHECK(m < 16) << "m=" << m;
        const uint16_t mask = UINT16_MAX >> (15-m);
        return rank1(uint16_t(v & mask));
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given 16-bit value.
    ///
    /// The exact rank value computed is that of the interval [l,m] in
    /// zero-based LSBF order.
    ///
    /// \param v the value in question
    /// \param l the least significant bit (from which the counting starts)
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    inline constexpr uint8_t rank1(uint16_t v, uint8_t l, uint8_t m) {
        DCHECK(l < 16 && m < 16 && l <= m) << "l=" << l << ",m=" << m;
        const uint16_t mask_m = UINT16_MAX >> (15-m);
        const uint16_t mask_l = UINT16_MAX << l;
        return rank1(uint16_t(v & mask_m & mask_l));
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given 32-bit value.
    ///
    /// The exact rank value computed is that of the interval [0,m] in
    /// zero-based LSBF order.
    ///
    /// \param v the value in question
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    inline constexpr uint8_t rank1(uint32_t v, uint8_t m) {
        DCHECK(m < 32) << "m=" << m;
        const uint32_t mask = UINT32_MAX >> (31-m);
        return rank1(uint32_t(v & mask));
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given 32-bit value.
    ///
    /// The exact rank value computed is that of the interval [l,m] in
    /// zero-based LSBF order.
    ///
    /// \param v the value in question
    /// \param l the least significant bit (from which the counting starts)
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    inline constexpr uint8_t rank1(uint32_t v, uint8_t l, uint8_t m) {
        DCHECK(l < 32 && m < 32 && l <= m) << "l=" << l << ",m=" << m;
        const uint32_t mask_m = UINT32_MAX >> (31-m);
        const uint32_t mask_l = UINT32_MAX << l;
        return rank1(uint32_t(v & mask_m & mask_l));
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given 64-bit value.
    ///
    /// The exact rank value computed is that of the interval [0,m] in
    /// zero-based LSBF order.
    ///
    /// \param v the value in question
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    inline constexpr uint8_t rank1(uint64_t v, uint8_t m) {
        DCHECK(m < 64) << "m=" << m;
        const uint64_t mask = UINT64_MAX >> (63-m);
        return rank1(uint64_t(v & mask));
    }

    /// \brief Computes the amount of 1-bits in an interval of the
    ///        binary representation of the given 64-bit value.
    ///
    /// The exact rank value computed is that of the interval [l,m] in
    /// zero-based LSBF order.
    ///
    /// \param v the value in question
    /// \param l the least significant bit (from which the counting starts)
    /// \param m the most significant bit (up until which to count)
    /// \return the amount of 1-bits in the given interval
    inline constexpr uint8_t rank1(uint64_t v, uint8_t l, uint8_t m) {
        DCHECK(l < 64 && m < 64 && l <= m) << "l=" << l << ",m=" << m;
        const uint64_t mask_m = UINT64_MAX >> (63-m);
        const uint64_t mask_l = UINT64_MAX << l;
        return rank1(uint64_t(v & mask_m & mask_l));
    }

    inline constexpr uint8_t rank0(uint8_t v) {
        return uint8_t(8) - rank1(v);
    }

    inline constexpr uint8_t rank0(uint16_t v) {
        return uint8_t(16) - rank1(v);
    }

    inline constexpr uint8_t rank0(uint32_t v) {
        return uint8_t(32) - rank1(v);
    }

    inline constexpr uint8_t rank0(uint64_t v) {
        return uint8_t(64) - rank1(v);
    }

    template<typename T>
    inline constexpr uint8_t rank0(T v, uint8_t m) {
        return (m + 1) - rank1(v, m);
    }

    template<typename T>
    inline constexpr uint8_t rank0(T v, uint8_t l, uint8_t m) {
        return (m - l + 1) - rank1(v, l, m);
    }

} //ns
