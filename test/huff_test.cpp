#include <glog/logging.h>
#include <gtest/gtest.h>
#include <cstring>
#include <bitset>
#include <algorithm>
#include "tudocomp/coders/HuffmanCoder.hpp"

void test_huffmantable_storing(const std::string& text) {
	using namespace tdc::huff;
	if(text.length() <=  1) return;
	extended_huffmantable table = gen_huffmantable(text);
	std::stringstream ss;
	{
		tdc::io::Output out(ss);
		tdc::io::BitOStream bit_os(out);
		huffmantable_encode(bit_os, table);
	}

		tdc::io::Input in(ss);
		tdc::io::BitIStream bit_in(in);
		huffmantable tab2 = huffmantable_decode(bit_in);

	DCHECK_EQ(table.longest, tab2.longest);
	DCHECK_EQ(table.alphabet_size, tab2.alphabet_size);
	for(size_t i = 0; i < table.longest; ++i) {
		DCHECK_EQ(table.numl[i], tab2.numl[i]);
	}
	for(size_t i = 0; i < table.alphabet_size; ++i) {
		DCHECK_EQ(table.ordered_map_from_effective[i], tab2.ordered_map_from_effective[i]);
	}
	const uint8_t*const ordered_codelengths2 = gen_ordered_codelength(table.alphabet_size, table.numl, table.longest);
	for(size_t i = 0; i < table.alphabet_size; ++i) {
		DCHECK_EQ(ordered_codelengths2[i], table.ordered_codelengths[i]);
	}
	delete [] ordered_codelengths2;
}

void test_huffmantable_coding(const std::string& text) {
	using namespace tdc::huff;
	if(text.length() <=  1) return;
	extended_huffmantable table = gen_huffmantable(text);
	//encode
	std::stringstream input(text);
	std::stringstream output;

	{//write
		tdc::io::Output out(output);
		tdc::io::BitOStream bit_os(out);
		huffman_encode(input, bit_os, text.length(), table.ordered_map_from_effective, table.ordered_codelengths, table.alphabet_size, table.codewords);
	}

	{//read
		tdc::io::Input in(output);
		tdc::io::BitIStream bit_in(in);

	//decode
	input.clear();
	input.str(std::string{});
	huffman_decode(
			bit_in,
			input,
			table.ordered_map_from_effective,
			table.ordered_codelengths,
			table.alphabet_size,
			table.numl,
			table.longest);
	}
	ASSERT_EQ(input.str(), text);
}

void test_huff(const std::string& text) {
	using namespace tdc::huff;
	if(text.length() <=  1) return;
	test_huffmantable_coding(text);
	test_huffmantable_storing(text);

	std::stringstream soutput;
	std::stringstream sback;
	{
		tdc::io::Input in(text);
		tdc::io::Output out(soutput);
		encode(in, out);
	}
	{

		std::string s = soutput.str();
		tdc::io::Input in(s);
		tdc::io::Output out(sback);
		decode(in, out);
	}
	DCHECK_EQ(text, sback.str());
}

#include "test/util.hpp"
TEST(huffman, stringgenerators) {
	std::function<void(std::string&)> func(test_huff);
	test::on_string_generators(func,20);
}


// #include "tudocomp/util/Generators.hpp"
// TEST(Sandbox, example) {
//
// 	{
// 		std::string s = thue_morse_word(10);
// 		// tdc::io::Input input(s);
// 		// auto iview = input.as_view();
// 		// auto iview2 = input.as_view();
// 		// DCHECK_EQ(iview.size(), iview2.size());
// 		test(s);
// 	}
// 	// for(size_t i = 8; i < n; ++i) {
// 	// 	std::cout << "String " << i << std::endl;
// 	// 	std::string s = thue_morse_word(i);
// 	// 	test(s);
// 	// }
// 	size_t n = 100;
// 	for(size_t i = 2; i < n; ++i) {
// 	 	for(size_t j = 0; j < i; ++j) {
// 	 		std::string s = random_uniform(2*i,Ranges::numbers,j);
// 			DLOG(INFO) << "i = " << i << " j = " << j;
// 				test(s);
// 		}
// 	}
// 	n=20;
// 	for(size_t i = 2; i < n; ++i) {
// 	 	for(size_t j = 0; j < 2+50/(i+1); ++j) {
// 	 		std::string s = random_uniform(1<<i,Ranges::numbers,j);
// 			DLOG(INFO) << "i = " << i << " j = " << j;
// 				test(s);
// 		}
// 	}
//
// }
