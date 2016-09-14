#ifndef TC_TEST_UTIL_H
#define TC_TEST_UTIL_H

#include "test_util.h"
#include <tudocomp/tudocomp.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/AlgorithmStringParser.hpp>
#include <tudocomp/Registry.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util/View.hpp>
#include <string>
#include <memory>

using tudocomp::Env;
using tudocomp::Input;
using tudocomp::Output;
using tudocomp::create_algo_with_registry;
using tudocomp::Registry;
using tudocomp::View;

namespace test {
    template<class C>
    struct CompressResult {
    private:
        Registry m_registry;
    public:
        std::vector<uint8_t> bytes;
        std::string str;
        std::string orginal_text;
        std::string options;

        CompressResult(const Registry& registry,
                       std::vector<uint8_t>&& p_bytes,
                       std::string&& p_str,
                       std::string&& p_original,
                       std::string&& p_options):
            m_registry(registry),
            bytes(std::move(p_bytes)),
            str(std::move(p_str)),
            orginal_text(std::move(p_original)),
            options(std::move(p_options)) {}

        void assert_decompress() {
            std::vector<uint8_t> decoded_buffer;
            {
                Input text_in = Input::from_memory(bytes);
                Output decoded_out = Output::from_memory(decoded_buffer);

                auto compressor = create_algo_with_registry<C>(options, m_registry);

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

                auto compressor = create_algo_with_registry<C>(options, m_registry);

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
        Registry m_registry;
    public:
        inline RoundTrip(const std::string& options = "",
                         const Registry& registry = Registry()):
            m_options(options),
            m_registry(registry)
        {
        }

        CompressResult<C> compress(const std::string& text) {
            std::vector<uint8_t> encoded_buffer;
            {
                Input text_in = Input::from_memory(text);
                Output encoded_out = Output::from_memory(encoded_buffer);

                auto compressor = create_algo_with_registry<C>(m_options, m_registry);

                compressor.compress(text_in, encoded_out);
            }
            std::string s(encoded_buffer.begin(), encoded_buffer.end());
            return CompressResult<C> {
                m_registry,
                std::move(encoded_buffer),
                std::move(s),
                std::string(text),
                std::string(m_options),
            };
        }
    };

    template<class T>
    inline CompressResult<T> compress(const std::string& text,
                                      const std::string& options = "",
                                      const Registry& registry = Registry()) {
        return RoundTrip<T>(options, registry).compress(text);
    }

    template<class T>
    inline void roundtrip(const std::string& original_text,
                          const std::string& expected_compressed_text,
                          const std::string& options = "",
                          const Registry& registry = Registry()) {
        auto e = RoundTrip<T>(options, registry).compress(original_text);
        auto& compressed_text = e.str;
        ASSERT_EQ(expected_compressed_text, compressed_text);
        e.assert_decompress();
    }

    template<class T>
    inline void roundtrip(const std::string& original_text,
                          const std::vector<uint8_t>& expected_compressed_text,
                          const std::string& options = "",
                          const Registry& registry = Registry()) {
        auto e = RoundTrip<T>(options, registry).compress(original_text);
        auto& compressed_text = e.bytes;
        ASSERT_EQ(expected_compressed_text, compressed_text);
        e.assert_decompress_bytes();
    }

}

#endif
