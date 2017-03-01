#include <gtest/gtest.h>

#include "test/util.hpp"
#include <tudocomp/compressors/RunLengthEncoder.hpp>

using namespace tdc;

void test_rle(const std::string& input) {
	std::stringstream rleout;
	std::stringstream rlein{input};
	rle_encode(rlein, rleout);
	rlein.clear();
	rlein.str(std::string{});
	rle_decode(rleout, rlein);
	DCHECK_EQ(input, rlein.str()) << "original: " << vec_to_debug_string(input) << " result: " << vec_to_debug_string(rlein.str());
}

TEST(RLE, string_test) {
	std::function<void(std::string&)> func(test_rle);
	test::on_string_generators(func,20);
}
