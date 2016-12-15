#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>

namespace tdc {
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
	#define tdc_likely(x)	__builtin_expect(x, 1)
	#define tdc_unlikely(x)  __builtin_expect(x, 0)
	#define tdc_warn_unused_result  __attribute__((warn_unused_result))
#else
	#define tdc_likely(x)	x
	#define tdc_unlikely(x)  x
	#define tdc_warn_unused_result
#endif
#ifdef NDEBUG
#define tdc_debug(x)
#else
#define tdc_debug(x) x
#ifdef DEBUG
#define tdc_hdebug(x) x
#else
#define tdc_hdebug(x)
#endif//DEBUG
#endif//NDEBUG
#ifndef tdc_hdebug
#define tdc_hdebug(x)
#endif

// do we want to log with env?
#define tdc_stats(x) x
// we do not want to log with env:
//#define tdc_stats(x) false

	typedef size_t len_t; // length type for text positions of the input
	typedef char literal_t; // data type of the alphabet
	typedef std::make_unsigned<literal_t>::type uliteral_t; // unsigned data type of the alphabet
	constexpr size_t uliteral_max = std::numeric_limits<uliteral_t>::max(); // the maximum value an input character can have
	constexpr size_t LEN_MAX = std::numeric_limits<len_t>::max(); // the maximum value an input character can have

	template<class T>
	inline size_t literal2int(const T& c) {
		return static_cast<size_t>(c);
	}
	template<>
	inline size_t literal2int(const literal_t& c) {
		return static_cast<size_t>(static_cast<uliteral_t>(c));
	}


}

