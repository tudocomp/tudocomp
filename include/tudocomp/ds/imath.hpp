#pragma once
#include <sdsl/bits.hpp>

//aka "w(i, j)" in Lu, Yeh
inline uint32_t substr_length(uint32_t i, uint32_t j)
{
    return j - i + 1;
}

//integer log2
inline uint32_t log2i(uint32_t n)
{
    uint32_t log = sdsl::bits::hi(n); //position of the highest 1-bit
    return (sdsl::bits::cnt32(n) > 1) ? (log + 1) : log; //round up, unless n is a round base 2 value
}

//integer division with rounding up
inline uint32_t idiv_ceil(uint32_t a, uint32_t b)
{
    return (a / b) + ((a % b) > 0);
}
