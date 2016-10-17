#ifndef UINT_T_HPP
#define UINT_T_HPP

#include <cstdint>
/** class for storing integers of arbitrary bits.
 * Useful values are 40,48, and 56.
 * Standard value is 40 bits since we can store values up to 1TiB and
 * address values up to 1TB with 40-bits integers and 40-bits pointers, respectively.
 */
template<int bits = 40>
class uint_t {
    static_assert(bits > 0, "bits must be non-negative");
    static_assert(bits < 65, "bits must be at most 64");
    uint64_t data : bits;

public:
    uint_t(uint64_t i) : data(i) {}
    uint_t() {}
    uint_t(const uint_t<bits>&& i) : data(i.data) {}
    uint_t(const uint_t<bits>& i) : data(i.data) {}
    inline uint_t& operator=(uint64_t data) { this->data = data; return *this; }
    inline uint_t& operator=(const uint_t<bits>& b) { this->data = b.data; return *this; }
    inline operator uint64_t() const { return data; }
    inline uint_t<bits>& operator--() { --data; return *this;}
    inline uint_t<bits>& operator++() { ++data; return *this;}
    inline uint64_t operator--(int) { return data--; }
    inline uint64_t operator++(int) { return data++; }
    inline uint_t<bits>& operator+=(const uint_t<bits>& b) { data += b.data; return *this; }
    inline uint_t<bits>& operator-=(const uint_t<bits>& b) { data -= b.data; return *this; }
    inline uint_t<bits>& operator*=(const uint_t<bits>& b) { data *= b.data; return *this; }
    inline bool operator==(const uint_t<bits>& b) const { return data == b.data; }
    inline bool operator!=(const uint_t<bits>& b) const { return data != b.data; }
    inline bool operator!=(uint64_t b) const { return data != b; }
    inline bool operator!=(int b) const { return data != b; }
    inline bool operator<=(const uint_t<bits>& b) const { return data <= b.data; }
    inline bool operator>=(const uint_t<bits>& b) const { return data >= b.data; }
    inline bool operator<(const uint_t<bits>& b) const { return data < b.data; }
    inline bool operator<(uint64_t b) const { return data < b; }
    inline bool operator>(uint64_t b) const { return data > b; }
    inline bool operator>(const uint_t<bits>& b) const { return data > b.data; }
} __attribute__((packed));

static_assert(sizeof(uint_t<8>)  == 1, "sanity check");
static_assert(sizeof(uint_t<16>) == 2, "sanity check");
static_assert(sizeof(uint_t<24>) == 3, "sanity check");
static_assert(sizeof(uint_t<32>) == 4, "sanity check");
static_assert(sizeof(uint_t<40>) == 5, "sanity check");
static_assert(sizeof(uint_t<48>) == 6, "sanity check");
static_assert(sizeof(uint_t<56>) == 7, "sanity check");
static_assert(sizeof(uint_t<64>) == 8, "sanity check");

static_assert(sizeof(uint_t<7>)  == 1, "sanity check");
static_assert(sizeof(uint_t<15>) == 2, "sanity check");
static_assert(sizeof(uint_t<23>) == 3, "sanity check");
static_assert(sizeof(uint_t<31>) == 4, "sanity check");
static_assert(sizeof(uint_t<39>) == 5, "sanity check");
static_assert(sizeof(uint_t<47>) == 6, "sanity check");
static_assert(sizeof(uint_t<55>) == 7, "sanity check");
static_assert(sizeof(uint_t<63>) == 8, "sanity check");

static_assert(sizeof(uint_t<9>)  == 2, "sanity check");
static_assert(sizeof(uint_t<17>) == 3, "sanity check");
static_assert(sizeof(uint_t<25>) == 4, "sanity check");
static_assert(sizeof(uint_t<33>) == 5, "sanity check");
static_assert(sizeof(uint_t<41>) == 6, "sanity check");
static_assert(sizeof(uint_t<49>) == 7, "sanity check");
static_assert(sizeof(uint_t<57>) == 8, "sanity check");

#endif /* UINT_T_HPP */
