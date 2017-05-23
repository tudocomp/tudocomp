#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <numeric>
#include <tudocomp/def.hpp>

namespace tdc {


/**
 * Encodes a character 'v' by Move-To-Front Coding
 * Needs and modifies a lookup table storing the last-used characters
 */
template<class value_type = uliteral_t>
value_type mtf_encode_char(const value_type v, value_type*const table, const size_t table_size) {
	for(size_t i = 0; i < table_size; ++i) {
		if(table[i] == v) {
			for(size_t j = i; j > 0; --j) {
				table[j] = table[j-1];
			}
			table[0] = v;
			return i;
		}
	}
	DCHECK(false) << v << "(" << static_cast<size_t>(static_cast<typename std::make_unsigned<value_type>::type>(v)) << " not in " << arr_to_debug_string(table,table_size);
	return 0;
}

/**
 * Decodes a character encoded as 'v' by Move-To-Front Coding
 * Needs and modifies a lookup table storing the last-used characters
 */
template<class value_type = uliteral_t>
value_type mtf_decode_char(const value_type v, value_type*const table) {
	const value_type return_value = table[v];
	for(size_t j = v; j > 0; --j) {
		table[j] = table[j-1];
	}
	table[0] = return_value;
	return return_value;
}

template<class char_type = uliteral_t>
void mtf_encode(std::basic_istream<char_type>& is, std::basic_ostream<char_type>& os) {
	typedef typename std::make_unsigned<char_type>::type value_type; // -> default: uint8_t
	static constexpr size_t table_size = std::numeric_limits<value_type>::max()+1;
	value_type table[table_size];
	std::iota(table, table+table_size, 0);

	char_type c;
	while(is.get(c)) {
		os << mtf_encode_char(static_cast<value_type>(c), table, table_size);
	}
}

template<class char_type = uliteral_t>
void mtf_decode(std::basic_istream<char_type>& is, std::basic_ostream<char_type>& os) {
	typedef typename std::make_unsigned<char_type>::type value_type; // -> default: uint8_t
	static constexpr size_t table_size = std::numeric_limits<value_type>::max()+1;
	value_type table[table_size];
	std::iota(table, table+table_size, 0);

	char_type c;
	while(is.get(c)) {
		os << mtf_decode_char(static_cast<value_type>(c), table);
	}
};

class MTFCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "mtf", "Move To Front Compressor");
        return m;
    }
    inline MTFCompressor(Env&& env)
		: Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
		auto is = input.as_stream();
		auto os = output.as_stream();
		mtf_encode(is,os);
	}
    inline virtual void decompress(Input& input, Output& output) override {
		auto is = input.as_stream();
		auto os = output.as_stream();
		mtf_decode(is,os);
	}
};


}//ns

