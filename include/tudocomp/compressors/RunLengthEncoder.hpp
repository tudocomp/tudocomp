#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/util/vbyte.hpp>
#include <tudocomp/Compressor.hpp>

namespace tdc {

/**
 * Encode a byte-stream with run length encoding
 * each run of the same character is substituted with two occurrences of the same character and the length of the run minus two,
 * encoded in vbyte coding.
 */
template<class char_type>
void rle_encode(std::basic_istream<char_type>& is, std::basic_ostream<char_type>& os, size_t offset = 0) {
	char_type prev;
	if(tdc_unlikely(!is.get(prev))) return;
	os << prev;
	char_type c;
	while(is.get(c)) {
		if(prev == c) {
			size_t run = 0;
			while(is.peek() == c) { ++run; is.get(); }
			os << c;
			write_vbyte(os, run+offset);
		} else {
			os << c;
		}
		prev = c;
	}
}
/**
 * Decodes a run length encoded stream
 */
template<class char_type>
void rle_decode(std::basic_istream<char_type>& is, std::basic_ostream<char_type>& os, size_t offset = 0) {
	char_type prev;
	if(tdc_unlikely(!is.get(prev))) return;
	os << prev;
	char_type c;
	while(is.get(c)) {
		if(prev == c) {
			size_t run = read_vbyte<size_t>(is)-offset;
			while(run-- > 0) { os << c; }
		}
		os << c;
		prev = c;
	}
}

class RunLengthEncoder : public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "rle",
            "Run-length encoding.");
        m.param("offset").primitive(0);
        return m;
    }
	const size_t m_offset;
    inline RunLengthEncoder(Config&& cfg)
		: Compressor(std::move(cfg)),
          m_offset(this->config().param("offset").as_uint()) {
    }

    inline virtual void compress(Input& input, Output& output) override {
		auto is = input.as_stream();
		auto os = output.as_stream();
		rle_encode(is,os,m_offset);
	}
    inline virtual void decompress(Input& input, Output& output) override {
		auto is = input.as_stream();
		auto os = output.as_stream();
		rle_decode(is,os,m_offset);
	}
};


}//ns

