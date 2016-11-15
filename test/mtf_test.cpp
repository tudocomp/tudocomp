
#include "tudocomp/compressors/MTFCompressor.hpp"
#include "test_util.hpp"
#include <gtest/gtest.h>

using namespace tdc;

void test_mtf(const std::string& input) {
	static constexpr size_t table_size = std::numeric_limits<uint8_t>::max();
	uint8_t table[table_size];
	std::iota(table, table+table_size, 0);
	
	std::string out;
	for(size_t i = 0; i < input.length(); ++i) {
		out += mtf_encode_char(static_cast<uint8_t>(input[i]), table, table_size);
	}
	std::iota(table, table+table_size, 0);
	std::string re;
	for(size_t i = 0; i < out.length(); ++i) {
		re += mtf_decode_char(static_cast<uint8_t>(out[i]), table);
	}
	DCHECK_EQ(input, re) << "original: " << vec_to_debug_string(input) << " result: " << vec_to_debug_string(re);
}

TEST(MTF, string_test) {
	std::function<void(std::string&)> func(test_mtf);
	test_on_string_generators(func,20);
}
