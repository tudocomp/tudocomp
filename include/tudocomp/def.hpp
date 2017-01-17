#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>

// assertions
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
	#define tdc_likely(x)	__builtin_expect(x, 1)
	#define tdc_unlikely(x)  __builtin_expect(x, 0)
	#define tdc_warn_unused_result  __attribute__((warn_unused_result))
#else
	#define tdc_likely(x)	x
	#define tdc_unlikely(x)  x
	#define tdc_warn_unused_result
#endif

// code compiled only in debug build (set build type to Debug)
#ifdef DEBUG
    #define IF_DEBUG(x) x
#else
    #define IF_DEBUG(x)
#endif

// code compiled only in paranoid debug build (pass -DPARANOID=1 to CMake)
#if defined(DEBUG) && defined(PARANOID)
    #define IF_PARANOID(x) x
#else
    #define IF_PARANOID(x)
#endif

// code compiled only if statistics tracking is enabled (default yes)
// (pass -DSTATS_DISABLED=1 to CMake to disable)
#ifdef STATS_DISABLED
    #define IF_STATS(x)
#else
    #define IF_STATS(x) x
#endif

namespace tdc {
	typedef uint32_t len_t; // length type for text positions of the input
	constexpr size_t LEN_MAX = std::numeric_limits<len_t>::max(); // the maximum value an input character can have
    constexpr size_t LEN_BITS = 8 * sizeof(len_t);

	typedef char literal_t; // data type of the alphabet

	typedef std::make_unsigned<literal_t>::type uliteral_t; // unsigned data type of the alphabet
	constexpr size_t uliteral_max = std::numeric_limits<uliteral_t>::max(); // the maximum value an input character can have

	template<class T>
	inline size_t literal2int(const T& c) {
		return static_cast<size_t>(c);
	}

	template<>
	inline size_t literal2int(const literal_t& c) {
		return static_cast<size_t>(static_cast<uliteral_t>(c));
	}
}

