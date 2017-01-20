#include "test/util.hpp"
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <cstring>
#include <bitset>
#include <algorithm>
#include <tudocomp/coders/HuffmanCoder.hpp>

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

	ASSERT_EQ(table.longest, tab2.longest);
	ASSERT_EQ(table.alphabet_size, tab2.alphabet_size);
	for(size_t i = 0; i < table.longest; ++i) {
		ASSERT_EQ(table.numl[i], tab2.numl[i]);
	}
	for(size_t i = 0; i < table.alphabet_size; ++i) {
		ASSERT_EQ(table.ordered_map_from_effective[i], tab2.ordered_map_from_effective[i]);
	}
	const uint8_t*const ordered_codelengths2 = gen_ordered_codelength(table.alphabet_size, table.numl, table.longest);
	for(size_t i = 0; i < table.alphabet_size; ++i) {
		ASSERT_EQ(ordered_codelengths2[i], table.ordered_codelengths[i]);
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
    ASSERT_EQ(input.str().size(), text.size());
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
	auto sbacks = sback.str();
	ASSERT_EQ(text, sbacks);
    ASSERT_EQ(text.size(), sbacks.size());
}

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

TEST(huff, nullbyte) {
    test_huff("hel\0lo"_v);
    test_huff("hello\0"_v);
    test_huff("hello\0\0\0"_v);
}

void test_binary_out(string_ref in, std::vector<uint64_t> packed_ints_out) {
    auto v = in;
    test::TestOutput o(false);
    {
        auto env = tdc::builder<HuffmanCoder>().env();
        typename HuffmanCoder::Encoder coder(std::move(env), o, ViewLiterals(v));

        for (auto c : v) {
            coder.encode(c, literal_r);
        }
    }
    auto res = o.result();
    test::assert_eq_binary(res, packed_ints_out);
}

TEST(Coder, binary_output) {
    test_binary_out("abcabacba"_v, {
        // leading 1 bit to signify start of header
        0b1,        1,

        // compressed int, table.longest == 2
        0b0,        1,
        0b0000010,  7, // == 2

        // compressed int, numl[0]
        0b0,        1,
        0b0000001,  7, // == 1

        // compressed int, numl[1]
        0b0,        1,
        0b0000010,  7, // == 2

        // compressed int, alphabet_size
        0b0,        1,
        0b0000011,  7, // == 3

        // characters
        0b01100001, 8, // == 97 == 'a'
        0b01100010, 8, // == 98 == 'b'
        0b01100011, 8, // == 99 == 'c'

        // huffman encoded symbols
        0b1,        1, // a
        0b00,       2, // b
        0b01,       2, // c
        0b1,        1, // a
        0b00,       2, // b
        0b1,        1, // a
        0b01,       2, // c
        0b00,       2, // b
        0b1,        1, // a

        // BitOStream termination
        0b000000111, 9,
    });
}

TEST(Coder, binary_output_null) {
    test_binary_out("ab\0aba\0ba"_v, {
        // leading 1 bit to signify start of header
        0b1,        1,

        // compressed int, table.longest == 2
        0b0,        1,
        0b0000010,  7, // == 2

        // compressed int, numl[0]
        0b0,        1,
        0b0000001,  7, // == 1

        // compressed int, numl[1]
        0b0,        1,
        0b0000010,  7, // == 2

        // compressed int, alphabet_size
        0b0,        1,
        0b0000011,  7, // == 3

        // characters
        0b01100001,  8, // == 97 == 'a'
        0b00000000,  8, // == 0 == '\0'
        0b01100010,  8, // == 98 == 'b'

        // huffman encoded symbols
        0b1,        1, // a
        0b01,       2, // b
        0b00,       2, // 0
        0b1,        1, // a
        0b01,       2, // b
        0b1,        1, // a
        0b00,       2, // 0
        0b01,       2, // b
        0b1,        1, // a

        // BitOStream termination
        0b000000111, 9,
    });
}

TEST(Coder, binary_output_null_2) {
    test_binary_out("a\0a\0ba\0a\0ba\0ba\0"_v, {
        0b1,        1,
        0b00000010, 8, // 2 codeword lengths
        0b00000001, 8, // 1 of 1
        0b00000010, 8, // 2 of 2
        0b00000011, 8, // 3 characters:
        '\0',       8, // => 0b1
        'a',        8, // => 0b00
        'b',        8, // => 0b01

        // 1. tuple
        0b00,       2,
        0b1,        1,

        // 2. tuple
        0b00,       2,
        0b1,        1,

        // 3. tuple
        0b01,       2,
        0b00,       2,
        0b1,        1,

        // 4. tuple
        0b00,       2,
        0b1,        1,

        // 5. tuple
        0b01,       2,
        0b00,       2,
        0b1,        1,

        // 6. tuple
        0b01,       2,
        0b00,       2,
        0b1,        1,

        // BitOStream term
        0b0000001,  7,

    });
}
