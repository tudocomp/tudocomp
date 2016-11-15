#ifndef BWT_HPP
#define BWT_HPP
#include <cstdint>
#include <tudocomp/util/View.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

	namespace bwt {

/** 
 * Computes the value BWT[i] of a text T given its suffix array SA
 * Runs in O(1) time since BWT[i] = SA[(T[i]-1) mod |SA|]
 */ 
template<typename text_t, typename sa_t>
inline typename text_t::value_type bwt(const text_t& text, const sa_t& sa, const size_t i) {
	return (sa[i] == 0) ? text[sa.size()-1] : text[sa[i]-1];
}

template<class T>
inline size_t char2int(const T& c) {
	return static_cast<size_t>(c);
}
template<>
inline size_t char2int(const char& c) {
	return static_cast<size_t>(static_cast<unsigned char>(c));
}

	typedef View::value_type char_t; // the type of input characters to handle
	constexpr size_t char_max = std::numeric_limits<char_t>::max(); // the maximum value an input character can have

/**
 * Computes the LF table used for decoding the BWT
 * Input is a BWT and its length
 */
template<typename bwt_t, typename len_t = uint32_t>
len_t* compute_LF(const bwt_t& bwt, const size_t bwt_length) {
	if(bwt_length == 0) return nullptr;
	len_t C[char_max+1] { 0 }; // alphabet counter
	for(auto& c : bwt) { 
		if(char2int(c) != char_max) {
			++C[char2int(c)+1]; 
		}
	}
	for(size_t i = 1; i < char_max; ++i) {
		C[i] += C[i-1];
	}
	DLOG(INFO) << "C: " << arr_to_debug_string(C,char_max);
	DCHECK_EQ(C[0],0); // no character preceeds 0
	DCHECK_EQ(C[1],1); // there is exactly only one '\0' byte

	len_t* LF { new len_t[bwt_length] };
	for(len_t i = 0; i < bwt_length; ++i) {
		DCHECK_LE(char2int(bwt[i]), char_max);
		LF[i] = C[char2int(bwt[i])];
		++C[char2int(bwt[i])];
	}

	DLOG(INFO) << "LF: " << arr_to_debug_string(LF, bwt_length);
	DCHECK([&] () { // unique invariant of the LF mapping
			assert_permutation(LF,bwt_length);
			for(len_t i = 0; i < bwt_length; ++i) 
			for(len_t j = i+1; j < bwt_length; ++j) {
			if(bwt[i] != bwt[j]) continue;
			DCHECK_LT(LF[i], LF[j]);
			}
			return true;
			}());

	return LF;
}


/**
 * Decodes a BWT
 * It is assumed that the BWT is stored in a container with access to operator[] and .size()
 */
template<typename bwt_t, typename len_t = uint32_t>
char_t* decode_bwt(const bwt_t& bwt) {
	const size_t bwt_length = bwt.size();
	DLOG(INFO) << "InputSize: " << bwt_length;
	if(bwt.empty()) return nullptr;
	if(bwt_length == 1) { // since there has to be a zero in each string, a string of length 1 is equal to '\0'
		char_t*const decoded_string { new char_t[1] };
		decoded_string[0] = 0;
		return decoded_string;
	}
	len_t*const LF { compute_LF(bwt, bwt_length) };

	char_t*const decoded_string = new char_t[bwt_length];
	decoded_string[bwt_length-1] = 0;
	len_t i = 0;
	for(len_t j = 1; j < bwt_length && bwt[i] != 0; ++j) {
		decoded_string[bwt_length - j-1] = bwt[i];
		i = LF[i];
		DCHECK( (bwt[i] == 0 && j+1 == bwt_length) || (bwt[i] != 0 && j+1 < bwt_length));
	}
	delete [] LF;
	return decoded_string;
}

}}//ns
#endif /* BWT_HPP */
