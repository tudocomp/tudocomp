#pragma once

#include <cstdint>
#include <tudocomp/util.hpp>

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
    DCHECK(k > 0) << "order must be at least one";
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

/// \brief Finds the position of the k-th 1-bit in the binary representation
///        of the given value.
///
/// \tparam uint_t the input value type
/// \param v the input value
/// \param l the bit (LSBF order) to start searching from
/// \param k the searched 1-bit
/// \return the position of the k-th 1-bit (LSBF and zero-based),
///         or \ref SELECT_FAIL if no such bit exists
template<typename uint_t>
inline constexpr uint8_t select1(uint_t v, uint8_t l, uint8_t k) {
    uint8_t pos = select1(v >> l, k);
    return (pos != SELECT_FAIL) ? (l + pos) : SELECT_FAIL;
}

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
    DCHECK(k > 0) << "order must be at least one";
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

/// \brief Finds the position of the k-th 0-bit in the binary representation
///        of the given value.
///
/// \tparam uint_t the input value type
/// \param v the input value
/// \param l the bit (LSBF order) to start searching from
/// \param k the searched 0-bit
/// \return the position of the k-th 0-bit (LSBF and zero-based),
///         or \ref SELECT_FAIL if no such bit exists
template<typename uint_t>
inline constexpr uint8_t select0(uint_t v, uint8_t l, uint8_t k) {
    uint8_t pos = select0(v >> l, k);
    if(pos != SELECT_FAIL) {
        pos += l;
        return (pos <= msbf<uint_t>::pos) ? pos : SELECT_FAIL; //TODO: throw error?
    } else {
        return SELECT_FAIL; //TODO: throw error?
    }
}

}
