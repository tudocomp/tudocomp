#pragma once

#include <tudocomp/def.hpp>
#include <limits>

namespace tdc {
    /// \brief Represents a generic range of positive integers
    ///
    /// Ranges have a minimum and a maximum value. They are used as hints for a
    /// \ref tdc::Coder for it to determine how to encode certain values.
    /// Correspondingly implemented coders can use this information to encode
    /// more efficiently, therefore it is advised to feed it ranges as precise
    /// as possible for every value to encode.
    class Range {
    private:
        size_t m_min;
        size_t m_max;

    public:
        /// \brief Constructs a range from zero to a maximum value
        /// \param max the range's maximum value
        inline Range(size_t max) : m_min(0), m_max(max) {}

        /// \brief Constructs a range
        /// \param min the range's minimum value
        /// \param max the range's maximum value
        inline Range(size_t min, size_t max) : m_min(min), m_max(max) {}

        /// \brief Yields the range's minimum value
        /// \return the range's minimum value
        inline size_t min() const { return m_min; }

        /// \brief Yields the range's maximum value
        /// \return the range's maximum value
        inline size_t max() const { return m_max; }

        /// \brief Yields the difference between the range's minimum and maximum
        ///        values
        /// \return the difference between the range's minimum and maximum
        ///         values
        inline size_t delta() const { return m_max - m_min; }
    };

    /// \brief Represents a range of positive integers that tend to be
    ///        distributed towards the minimum
    ///
    /// Correspondingly implemented coders can use this information to encode
    /// more efficiently, therefore it is advised to feed it ranges as precise
    /// as possible for every value to encode.
    class MinDistributedRange : public Range {
    public:
        /// \brief Constructs a range from zero to a maximum value
        /// \param max the range's maximum value
        inline MinDistributedRange(size_t max) : Range(0, max) {}

        /// \brief Constructs a range
        /// \param min the range's minimum value
        /// \param max the range's maximum value
        inline MinDistributedRange(size_t min, size_t max) : Range(min, max) {}
    };

    /// \brief Represents a range of valid values for a certain type
    /// \tparam T the value type
    template<typename T>
    class TypeRange : public Range {
    public:
        /// \brief Constructs a range for the type
        TypeRange() : Range(0, std::numeric_limits<T>::max()) {}
    };

    /// \brief Represents a compiler-level fixed range
    template<size_t t_min, size_t t_max>
    class FixedRange : public Range {
    public:
        /// \brief Constructs a range with the fixed values
        FixedRange() : Range(t_min, t_max)  {}
    };

    /// \brief Represents the range of valid \ref tdc::uliteral_t values
    class LiteralRange : public TypeRange<uliteral_t> {
    public:
        LiteralRange(): TypeRange<uliteral_t>() {}
    };

    /// \brief Represents the range of valid \ref tdc::len_t values
    class LengthRange  : public TypeRange<len_t> {
    public:
        LengthRange(): TypeRange<len_t>() {}
    };

    /// \brief Represents the range of bit values, ie `0` to `1`
    using BitRange = FixedRange<0, 1>;

    /// \brief Global predefined range for the `size_t`.
    const TypeRange<size_t> size_r;

    /// \brief Global predefined range for bits (`0` or `1`).
    const BitRange bit_r;

    /// \brief Global predefined reange for literals.
    const LiteralRange literal_r, uliteral_r;

    /// \brief Global predefined range for `len_t`.
    const LengthRange len_r;
}

