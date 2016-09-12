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
	uint_t() {}
	uint_t(uint64_t val) : data(val) {}
	inline uint_t& operator=(uint64_t& data) { this->data = data; return *this; }
	inline uint_t& operator=(uint64_t&& data) { this->data = data; return *this; }
	inline uint_t& operator=(uint_t<bits>& b) { this->data = b.data; return *this; }
	inline operator uint64_t() const { return data; }   
    inline uint_t<bits>& operator--() { --data; return *this;}
    inline uint_t<bits>& operator++() { ++data; return *this;}
    inline uint64_t& operator--(int) { return data--; }
    inline uint64_t& operator++(int) { return data++; }
    inline uint_t<bits>& operator+=(const uint_t<bits>& b) { data += b.data; return *this; }
    inline uint_t<bits>& operator-=(const uint_t<bits>& b) { data -= b.data; return *this; }
    inline uint_t<bits>& operator*=(const uint_t<bits>& b) { data *= b.data; return *this; }
    inline bool operator==(const uint_t<bits>& b) const { return data == b.data; }
    inline bool operator!=(const uint_t<bits>& b) const { return data != b.data; }
    inline bool operator<=(const uint_t<bits>& b) const { return data <= b.data; }
    inline bool operator>=(const uint_t<bits>& b) const { return data >= b.data; }
    inline bool operator<(const uint_t<bits>& b) const { return data < b.data; }
    inline bool operator>(const uint_t<bits>& b) const { return data > b.data; }
} __attribute__((packed));

#include <functional>
namespace std { 
  template <int bits>
	struct hash<uint_t<bits>> {
  size_t operator()(const uint_t<bits>& x) const { 
		return static_cast<uint64_t>(x);
	} 
};
  // or
  // hash<X>::operator()(X x) const { return hash<int>()(x.id); }     // works for g++ 4.7, but not for VC10 
}//ns                                                                 


#endif /* UINT_T_HPP */
