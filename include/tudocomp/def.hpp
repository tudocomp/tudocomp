#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>

#include <tudocomp/ds/uint_t.hpp>

// assertions
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
    /// Provides a hint to the compiler that `x` is expected to resolve to
    /// \e true.
    #define tdc_likely(x)    __builtin_expect((x) != 0, 1)
    /// Provides a hint to the compiler that `x` is expected to resolve to
    /// \e false.
    #define tdc_unlikely(x)  __builtin_expect((x) != 0, 0)
#else
    /// Provides a hint to the compiler that `x` is expected to resolve to
    /// \e true.
    #define tdc_likely(x)    x
    /// Provides a hint to the compiler that `x` is expected to resolve to
    /// \e false.
    #define tdc_unlikely(x)  x
#endif

// code compiled only in debug build (set build type to Debug)
#ifdef DEBUG
    /// `x` is compiled only in debug builds.
    #define IF_DEBUG(x) x
#else
    /// `x` is compiled only in debug builds.
    #define IF_DEBUG(x)

    #if defined(GOOGLE_STRIP_LOG)
    #undef GOOGLE_STRIP_LOG
    #endif

	#define GOOGLE_STRIP_LOG 1 // no google logging
#endif

// code compiled only in paranoid debug build (pass -DPARANOID=1 to CMake)
#if defined(DEBUG) && defined(PARANOID)
    /// `x` is compiled only in debug builds and when the `PARANOID` macro
    /// is defined.
    #define IF_PARANOID(x) x
#else
    /// `x` is compiled only in debug builds and when the `PARANOID` macro
    /// is defined.
    #define IF_PARANOID(x)
#endif

// code compiled only if statistics tracking is enabled (default yes)
// (pass -DSTATS_DISABLED=1 to CMake to disable)
#ifdef STATS_DISABLED
    /// `x` is compiled only when the `STATS_DISABLED` macro is undefined.
    #define IF_STATS(x)
#else
    /// `x` is compiled only when the `STATS_DISABLED` macro is undefined.
    #define IF_STATS(x) x
#endif

namespace tdc {
    /// \cond INTERNAL
    template<typename T, class X = void>
    struct FastIntType {
        // <not defined>
    };
    template<size_t N>
    struct FastIntType<uint_impl_t<N>, std::enable_if_t<(N <= 32)>> {
        using Type = uint32_t;
    };
    template<size_t N>
    struct FastIntType<uint_impl_t<N>, std::enable_if_t<(N > 32)>> {
        using Type = uint64_t;
    };
    template<>
    struct FastIntType<uint32_t> {
        using Type = uint32_t;
    };
    template<>
    struct FastIntType<uint64_t> {
        using Type = uint64_t;
    };
    /// \endcond

    /// Type to represent integer values in the size range of `actual_type`
    /// that may require more Bits than it, while being faster.
    template<typename actual_type>
    using fast_t = typename FastIntType<actual_type>::Type;

    /// Type to represent an bit-compact length value.
    ///
    /// This type can be defined to take up less bytes than a `size_t`,
    /// but might have worse performance for arithmetic operations. It should
    /// only be used for storing many integers in a data structure,
    /// to reduce memory usage.
    ///
    /// For fast arithmetic operations, prefer a cast to `len_t`
    /// or `size_t`.
#ifdef LEN_BITS
    using len_compact_t = uint_t<LEN_BITS>;
#else
    using len_compact_t = uint32_t;
#endif

    /// Type to represent an length value.
    ///
    /// This type is defined to take up at least as many bytes as a `len_compact_t`,
    /// but is guaranteed to be a fast integer type with the size of a machine word.
    /// It should be used for fast arithmetic operations and local variables.
    ///
    /// For a more compact memory footprint in array data structures,
    /// prefer a cast to `len_compact_t`.
    using len_t = fast_t<len_compact_t>;

    /// The maximum value of \ref len_compact_t.
    constexpr size_t INDEX_MAX = std::numeric_limits<len_compact_t>::max();

    /// The amount of bits required to store the binary representation of a
    /// value of type \ref len_compact_t.
    constexpr size_t INDEX_BITS = 8 * sizeof(len_compact_t);

    /// The maximum value of \ref len_t.
    constexpr size_t INDEX_FAST_MAX = std::numeric_limits<len_t>::max();

    /// The amount of bits required to store the binary representation of a
    /// value of type \ref len_t.
    constexpr size_t INDEX_FAST_BITS = 8 * sizeof(len_t);

    /// Type to represent signed single literals.
    typedef uint8_t uliteral_t;

    /// The maximum value of \ref uliteral_t.
    constexpr size_t ULITERAL_MAX = std::numeric_limits<uliteral_t>::max();

    /// Converts a literal to an integer value as if unsigned.
    ///
    /// \tparam T the integer type.
    /// \param c the literal.
    /// \return the corresponding unsigned integer value.
    template<typename T = size_t>
    constexpr T literal2int(uliteral_t c) {
        return std::make_unsigned_t<T>(c);
    }

    /// Converts an integer value to a literal as if unsigned.
    ///
    /// \tparam T the integer type.
    /// \param c the integer value.
    /// \return the corresponding literal.
    template<typename T = size_t>
    constexpr uliteral_t int2literal(const T& c) {
        return std::make_unsigned_t<T>(c);
    }
}
