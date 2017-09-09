#pragma once

#include <cstdint>

namespace tdc {

// thiss linear shifting approach is indeed the fastest, compared with
// a binary search approach and one using __builtin_ffs / __builtin_ctz

/// \brief Returned by \ref select0 and \ref select1 in case the searched
///        bit does not exist in the given input value.
constexpr uint8_t SELECT_FAIL = 0xFF;

/// \brief Finds the position of the k-th 1-bit in the binary representation
///        of the given value.
///
/// The search starts with the least significant bit.
///
/// \tparam uint_t the input value type
/// \param v the input value
/// \param k the searched 1-bit
/// \return the position of the k-th 1-bit (LSBF and zero-based),
///         or \ref SELECT_FAIL if no such bit exists
template<typename uint_t>
inline constexpr uint8_t select1(uint_t v, uint8_t k) {
    DCHECK(k > 0) << "k must be at least zero";
    uint8_t pos = 0;
    while(v) {
        if(v&1) {
            if(!--k) return pos;
        }
        ++pos;
        v >>= 1;
    }
    return SELECT_FAIL; //TODO: throw error?
}

/// \cond INTERNAL
template<typename uint_t> struct msbf;
template<> struct msbf<uint8_t>  { static constexpr uint8_t pos = 7; };
template<> struct msbf<uint16_t> { static constexpr uint8_t pos = 15; };
template<> struct msbf<uint32_t> { static constexpr uint8_t pos = 31; };
template<> struct msbf<uint64_t> { static constexpr uint8_t pos = 63; };
/// \endcond

/// \brief Finds the position of the k-th 0-bit in the binary representation
///        of the given value.
///
/// The search starts with the least significant bit.
///
/// \tparam uint_t the input value type
/// \param v the input value
/// \param k the searched 0-bit
/// \return the position of the k-th 0-bit (LSBF and zero-based),
///         or \ref SELECT_FAIL if no such bit exists
template<typename uint_t>
inline constexpr uint8_t select0(uint_t v, uint8_t k) {
    DCHECK(k > 0) << "k must be at least zero";
    uint8_t pos = 0;
    while(v) {
        if(!(v&1)) {
            if(!--k) return pos;
        }
        ++pos;
        v >>= 1;
    }
    pos += k-1;
    return (pos <= msbf<uint_t>::pos) ? pos : SELECT_FAIL; //TODO: throw error?
}

}
