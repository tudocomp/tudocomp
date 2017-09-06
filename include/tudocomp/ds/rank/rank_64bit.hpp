#pragma once

namespace tdc {

uint8_t rank1_8bit[] = {
    0, 1, 1, 2, 1, 2, 2, 3,
    1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7,
    5, 6, 6, 7, 6, 7, 7, 8
};

inline int rank1(uint8_t v) {
    return rank1_8bit[v];
}

inline int rank1(uint16_t v) {
    return rank1_8bit[uint8_t(v >> 8)] + rank1_8bit[uint8_t(v)];
}

inline int rank1(uint32_t v) {
    return rank1_8bit[uint8_t(v >> 24)] + rank1_8bit[uint8_t(v >> 16)] +
           rank1_8bit[uint8_t(v >>  8)] + rank1_8bit[uint8_t(v)];
}

inline int rank1(uint64_t v) {
    return rank1_8bit[uint8_t(v >> 56)] + rank1_8bit[uint8_t(v >> 48)] +
           rank1_8bit[uint8_t(v >> 40)] + rank1_8bit[uint8_t(v >> 32)] +
           rank1_8bit[uint8_t(v >> 24)] + rank1_8bit[uint8_t(v >> 16)] +
           rank1_8bit[uint8_t(v >>  8)] + rank1_8bit[uint8_t(v)];
}

inline int rank1(uint8_t v, uint8_t r) {
    // rank from least significant bit up to bit r (MSBF)
    return rank1(uint8_t(v & ((1ULL << r) - 1ULL)));
}

inline int rank1(uint16_t v, uint8_t r) {
    // rank from least significant bit up to bit r (MSBF)
    return rank1(uint16_t(v & ((1ULL << r) - 1ULL)));
}

inline int rank1(uint32_t v, uint8_t r) {
    // rank from least significant bit up to bit r (MSBF)
    return rank1(uint32_t(v & ((1ULL << r) - 1ULL)));
}

inline int rank1(uint64_t v, uint8_t r) {
    // rank from least significant bit up to bit r (MSBF)
    return rank1(uint64_t(v & ((1ULL << r) - 1ULL)));
}

inline int rank1(uint8_t v, uint8_t l, uint8_t r) {
    // rank between bit l and bit r (MSBF)
    return rank1(uint8_t((v >> l) & ((1ULL << (r - l)) - 1ULL)));
}

inline int rank1(uint16_t v, uint8_t l, uint8_t r) {
    // rank between bit l and bit r (MSBF)
    return rank1(uint16_t((v >> l) & ((1ULL << (r - l)) - 1ULL)));
}

inline int rank1(uint32_t v, uint8_t l, uint8_t r) {
    // rank between bit l and bit r (MSBF)
    return rank1(uint32_t((v >> l) & ((1ULL << (r - l)) - 1ULL)));
}

inline int rank1(uint64_t v, uint8_t l, uint8_t r) {
    // rank between bit l and bit r (MSBF)
    return rank1(uint64_t((v >> l) & ((1ULL << (r - l)) - 1ULL)));
}

}

