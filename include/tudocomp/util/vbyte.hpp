#ifndef VBYTE_HPP
#define VBYTE_HPP
#include <tudocomp/util.hpp>

namespace tdc {

/** 
 * Reads an integer stored as a bunch of bytes in the vbyte-encoding.
 */
template<class int_t>
inline int_t read_vbyte(std::istream& is) {
	constexpr size_t data_width = sizeof(std::istream::char_type)*8 - 1; // each byte of a vbyte-array stores 7bits of data, 1bit for marking that this byte is not the end byte
	int_t ret = 0;
	uint8_t which_byte = 0;
	while(is.good()) {
		uint8_t byte = is.get();
		ret |= (byte & ((1UL<<data_width)-1))<<(data_width * which_byte++);
		if( !(byte & (1UL<<data_width))) return ret;
	}
	DCHECK(false) << "VByte ended without reading a byte with the most significant bit equals zero.";
	return 0;
}

/** 
 * Store an integer as a bunch of bytes. The highest bit determines whether a
 * byte is the last byte representing the integer
 */
template<class int_t>
inline void write_vbyte(std::ostream& os, int_t v) {
	constexpr size_t data_width = sizeof(std::ostream::char_type)*8 - 1; // each byte of a vbyte-array stores 7bits of data, 1bit for marking that this byte is not the end byte
	do {
		uint8_t byte = v & ((1UL<<data_width)-1);
		v >>= data_width;
		if(v > 0) byte |= (1UL<<data_width);
		os << byte;
	} while(v > 0);
}


}//ns
#endif /* VBYTE_HPP */
