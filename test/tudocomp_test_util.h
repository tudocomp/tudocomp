#ifndef TC_TEST_UTIL_H
#define TC_TEST_UTIL_H

#include "test_util.h"
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/io.h>
#include <string>
#include <memory>

using tudocomp::Env;
using tudocomp::Input;
using tudocomp::Output;
using tudocomp::create_algo;

namespace test {
    template<class C>
    struct CompressResult {
        std::vector<uint8_t> bytes;
        std::string str;
        std::string orginal_text;
        std::string options;

        void assert_decompress() {
            std::vector<uint8_t> decoded_buffer;
            {
                Input text_in = Input::from_memory(bytes);
                Output decoded_out = Output::from_memory(decoded_buffer);

                auto compressor = create_algo<C>(options);

                compressor.decompress(text_in, decoded_out);
            }
            std::string decompressed_text {
                decoded_buffer.begin(),
                decoded_buffer.end(),
            };
            ASSERT_EQ(orginal_text, decompressed_text);
        }

        void assert_decompress_bytes() {
            std::vector<uint8_t> decompressed_bytes;
            {
                Input text_in = Input::from_memory(bytes);
                Output decoded_out = Output::from_memory(decompressed_bytes);

                auto compressor = create_algo<C>(options);

                compressor.decompress(text_in, decoded_out);
            }
            std::vector<uint8_t> orginal_bytes {
                orginal_text.begin(),
                orginal_text.end(),
            };
            ASSERT_EQ(orginal_bytes, decompressed_bytes);
        }
    };

    template<class C>
    class RoundTrip {
        std::string m_options;
    public:
        inline RoundTrip(const std::string& options = ""):
            m_options(options)
        {
        }

        CompressResult<C> compress(const std::string& text) {
            std::vector<uint8_t> encoded_buffer;
            {
                Input text_in = Input::from_memory(text);
                Output encoded_out = Output::from_memory(encoded_buffer);

                //create_algo
                auto compressor = create_algo<C>(m_options);

                compressor.compress(text_in, encoded_out);
            }
            return CompressResult<C> {
                encoded_buffer,
                std::string(encoded_buffer.begin(), encoded_buffer.end()),
                text,
                m_options,
            };
        }
    };

    template<class T>
    inline CompressResult<T> compress(const std::string& text, const std::string& options = "") {
        return RoundTrip<T>(options).compress(text);
    }

    template<class T>
    inline void roundtrip(const std::string& original_text,
                          const std::string& expected_compressed_text,
                          const std::string& options = "") {
        auto e = RoundTrip<T>(options).compress(original_text);
        auto& compressed_text = e.str;
        ASSERT_EQ(expected_compressed_text, compressed_text);
        e.assert_decompress();
    }

    template<class T>
    inline void roundtrip(const std::string& original_text,
                          const std::vector<uint8_t>& expected_compressed_text,
                          const std::string& options = "") {
        auto e = RoundTrip<T>(options).compress(original_text);
        auto& compressed_text = e.bytes;
        ASSERT_EQ(expected_compressed_text, compressed_text);
        e.assert_decompress_bytes();
    }

}

#endif
