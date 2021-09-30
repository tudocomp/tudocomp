#pragma once

#include <cstdint>
#include <type_traits>

#include "sdsl_bits.hpp"

namespace tdc {
    using sdsl_bits = tdc_sdsl::bits;

    /// \brief Computes the highest set bit in an integer variable
    inline constexpr uint_fast8_t bits_hi(uint64_t x) {
        return x == 0 ? 0 : 64 - __builtin_clzll(x);
    }

    /// \brief Computes the number of bits required to store the given integer
    /// value.
    ///
    /// This is equivalent to the binary logarithm rounded up to the next integer.
    ///
    /// Examples:
    /// - `bits_for(0b0) == 1`
    /// - `bits_for(0b1) == 1`
    /// - `bits_for(0b10) == 2`
    /// - `bits_for(0b11) == 2`
    /// - `bits_for(0b100) == 3`
    ///
    /// \param n The integer to be stored.
    /// \return The amount of bits required to store the value (guaranteed to be
    /// greater than zero).
    inline constexpr uint_fast8_t bits_for(size_t n) {
        return n == 0 ? 1U : bits_hi(n);
    }

    /// \cond INTERNAL
    namespace portable_arithmetic_shift {
        // Based on source: https://palotasb.wordpress.com/2016/12/29/fast-portable-c-cpp-arithmetic-shifting-power-of-two/

        template <typename T>
        constexpr auto builtin_shr(T value, int amount) noexcept
            -> decltype(value >> amount)
        {
            return value >> amount;
        }

        template <typename T>
        constexpr bool uses_arithmetic_shift = std::integral_constant<bool, builtin_shr(T(-1), 1) == -1>::value;

        template <typename T >
        constexpr T shift_by_portable(T value, int amount) noexcept
        {
            return value < 0 ?
            amount < 0 ? ~(~value >> -amount) : -(-value << amount) :
            amount < 0 ? value >> -amount : value << amount;
        }

        template <typename T >
        constexpr T shift_by_arithmetic(T value, int amount) noexcept
        {
            // Only use with negative T values when the compiler translates right shift to arithmetic shift instructions.
            return amount < 0 ? value >> -amount : value << amount;
        }

        template <typename T >
        constexpr T shift_right_by_portable(T value, unsigned int amount) noexcept
        {
            return value < 0
                ? ~(~value >> amount)
                :  value >> amount;
        }

        template <typename T >
        constexpr T shift_right_by_arithmetic(T value, unsigned int amount) noexcept
        {
            // Only use with negative T values when the compiler translates right shift to arithmetic shift instructions.
            return value >> amount;
        }

        template <typename T >
        constexpr T shift_left_by_portable(T value, unsigned int amount) noexcept
        {
            return value < 0
                ? -(-value << amount)
                : value << amount;
        }

        template <typename T >
        constexpr T shift_left_by_arithmetic(T value, unsigned int amount) noexcept
        {
            // Only use with negative T values when the compiler translates right shift to arithmetic shift instructions.
            return value << amount;
        }
    }
    /// \endcond INTERNAL

    /// A portable implementation of arithmetic (sign extending) shifting.
    ///
    /// Positive values for `amount` result in a left shift, negative values
    /// result in a right shift.
    ///
    /// This is needed because the shift operators have undefined/implementation defined
    /// behavior if used on negative numbers or with negative shift amounts.
    template <typename T >
    constexpr T shift_by(T value, int amount) noexcept
    {
        using namespace portable_arithmetic_shift;
        return uses_arithmetic_shift<T>
            ? shift_by_arithmetic(value, amount)
            : shift_by_portable(value, amount);
    }

    /// A portable implementation of arithmetic (sign extending) shifting.
    ///
    /// This is needed because the shift operators have undefined/implementation defined
    /// behavior if used on negative numbers or with negative shift amounts.
    template <typename T >
    constexpr T shift_right_by(T value, unsigned int amount) noexcept
    {
        using namespace portable_arithmetic_shift;
        return uses_arithmetic_shift<T>
            ? shift_right_by_arithmetic(value, amount)
            : shift_right_by_portable(value, amount);
    }

    /// A portable implementation of arithmetic (sign extending) shifting.
    ///
    /// This is needed because the shift operators have undefined/implementation defined
    /// behavior if used on negative numbers or with negative shift amounts.
    template <typename T >
    constexpr T shift_left_by(T value, unsigned int amount) noexcept
    {
        using namespace portable_arithmetic_shift;
        return uses_arithmetic_shift<T>
            ? shift_left_by_arithmetic(value, amount)
            : shift_left_by_portable(value, amount);
    }

    /// Advance a int pointer one `len` unit to the right.
    ///
    /// This is only defined for `len <= 64`
    inline void fast_move_right(const uint64_t*& word, uint8_t& offset, const uint8_t len)
    {
        offset += len;
        word += (offset >> 6); // advance pointer if offset is greater than 63
        offset &= 0x3F;        // bound offset to mod 64
    }

    /// Advance a int pointer one `len` unit to the left.
    ///
    /// This is only defined for `len <= 64`
    inline void fast_move_left(const uint64_t*& word, uint8_t& offset, const uint8_t len)
    {
        offset -= len;
        word += shift_right_by(int8_t(offset), 6); // advance pointer if offset is less than 0
        offset &= 0x3F;                            // bound offset to mod 64
    }

    /// Move a int pointer by `bits` bits.
    inline void fast_move_far(const uint64_t*& word, uint8_t& offset, const int64_t bits)
    {
        int64_t const far_offset = bits + uint64_t(offset);
        word += shift_right_by(far_offset, 6);
        offset = far_offset & 0x3F;
    }
}
