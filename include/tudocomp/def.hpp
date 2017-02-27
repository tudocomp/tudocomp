#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>

#include <tudocomp/ds/uint_t.hpp>

// assertions
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
    /// Provides a hint to the compiler that `x` is expected to resolve to
    /// \e true.
	#define tdc_likely(x)	__builtin_expect((x) != 0, 1)
    /// Provides a hint to the compiler that `x` is expected to resolve to
    /// \e false.
	#define tdc_unlikely(x)  __builtin_expect((x) != 0, 0)
#else
    /// Provides a hint to the compiler that `x` is expected to resolve to
    /// \e true.
	#define tdc_likely(x)	x
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
    /// Type to represent input lengths.
	typedef uint32_t len_t;

    /// The maximum value of \ref len_t.
	constexpr size_t LEN_MAX = std::numeric_limits<len_t>::max();

    /// The amount of bits required to store the binary representation of a
    /// value of type \ref len_t.
    constexpr size_t LEN_BITS = 8 * sizeof(len_t);

    /// Type to represent signed single literals.
	typedef char literal_t;

    /// Type to represent unsigned single literals.
	typedef std::make_unsigned<literal_t>::type uliteral_t;

    /// The maximum value of \ref uliteral_t.
	constexpr size_t ULITERAL_MAX = std::numeric_limits<uliteral_t>::max();

    /// Converts a literal to an unsigned integer value.
    ///
    /// \tparam the literal type.
    /// \param c the literal.
    /// \return the corresponding unsigned integer value.
	template<class T>
	inline size_t literal2int(const T& c) {
		return static_cast<size_t>(c);
	}

    /// Converts a signed literal to an unsigned integer value.
    ///
    /// \param c the literal.
    /// \return the corresponding unsigned integer value.
	template<>
	inline size_t literal2int(const literal_t& c) {
		return static_cast<size_t>(static_cast<uliteral_t>(c));
	}
}
