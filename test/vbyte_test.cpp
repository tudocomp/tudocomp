#include <gtest/gtest.h>
#include <tudocomp/util/vbyte.hpp>

#include "test/util.hpp"

using namespace tdc;

template<class int_t>
void test_vbyte(const int_t& i) {
	std::stringstream ss;
	write_vbyte(ss, i);
	DCHECK_EQ(read_vbyte<size_t>(ss), i) << ", contents of buffer: " << vec_to_debug_string(ss.str());
}

TEST(VByte, heavy) {
	for(size_t i = 0; i < 1ULL<<63; i=i*2+1) {
		test_vbyte(i);
	}
	for(size_t i = 0; i < 1ULL<<18; ++i) {
		test_vbyte(i);
	}
	for(size_t i = 1; i < 1ULL<<63; i<<=1) {
		test_vbyte(i);
	}

	std::stringstream ss;
	for(size_t i = 0; i < 1ULL<<10; ++i) {
		write_vbyte(ss,i);
	}
	for(size_t i = 0; i < 1ULL<<10; ++i) {
		DCHECK_EQ(read_vbyte<size_t>(ss), i) << ", contents of buffer: " << vec_to_debug_string(ss.str());
	}

}
