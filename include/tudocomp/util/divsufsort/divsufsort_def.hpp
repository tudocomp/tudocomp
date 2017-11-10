#pragma once

#include <tudocomp/def.hpp>

/// \cond INTERNAL
namespace tdc {
namespace libdivsufsort {

// core type definitions
using saidx_t = ssize_t;
using saint_t = int;
using sauchar_t = uliteral_t;

// flag for 64-bit
const bool divsufsort64 = (sizeof(saidx_t) > 4);

// common definitions
const saint_t lg_table[256]= {
 -1,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

#if SS_BLOCKSIZE == 0

template<typename T, typename X = void>
struct ilg_dispatch_t {};

template<typename T>
struct ilg_dispatch_t<T, typename std::enable_if<sizeof(T) * CHAR_BIT == 32>::type> {
    inline static saint_t calc(T x) {
        int32_t n = x;
        return (n & 0xffff0000) ?
            ((n & 0xff000000) ?
                24 + lg_table[(n >> 24) & 0xff] :
                16 + lg_table[(n >> 16) & 0xff]) :
            ((n & 0x0000ff00) ?
                8 + lg_table[(n >>  8) & 0xff] :
                0 + lg_table[(n >>  0) & 0xff]);
    }
};

template<typename T>
struct ilg_dispatch_t<T, typename std::enable_if<sizeof(T) * CHAR_BIT == 64>::type> {
    inline static saint_t calc(T x) {
        int64_t n = x;
        return (n >> 32) ?
            ((n >> 48) ?
                ((n >> 56) ?
                    56 + lg_table[(n >> 56) & 0xff] :
                    48 + lg_table[(n >> 48) & 0xff]) :
                ((n >> 40) ?
                    40 + lg_table[(n >> 40) & 0xff] :
                    32 + lg_table[(n >> 32) & 0xff])) :
            ((n & 0xffff0000) ?
                ((n & 0xff000000) ?
                    24 + lg_table[(n >> 24) & 0xff] :
                    16 + lg_table[(n >> 16) & 0xff]) :
                ((n & 0x0000ff00) ?
                    8 + lg_table[(n >>  8) & 0xff] :
                    0 + lg_table[(n >>  0) & 0xff]));
    }
};

template<typename idx_t>
inline saint_t ilg(idx_t n) {
    return ilg_dispatch_t<idx_t>::calc(n);
}

#else

inline saint_t ilg(saidx_t n) {
    #if SS_BLOCKSIZE < 256
      return lg_table[n];
    #else
      return (n & 0xff00) ?
              8 + lg_table[(n >> 8) & 0xff] :
              0 + lg_table[(n >> 0) & 0xff];
    #endif
}

#endif

}} //ns
///\endcond

