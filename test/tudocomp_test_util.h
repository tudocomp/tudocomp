#ifndef TC_TEST_UTIL_H
#define TC_TEST_UTIL_H

#include "test_util.h"
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.h>
#include <string>
#include <memory>

using tudocomp::Env;
using tudocomp::Input;
using tudocomp::Output;

namespace test {
    template<class C>
    struct CompressResult {
        std::vector<uint8_t> bytes;
        std::string str;
        std::string orginal_text;
        std::unique_ptr<Env> env_p;

        void assert_decompress() {
            std::vector<uint8_t> decoded_buffer;
            {
                Input text_in = Input::from_memory(bytes);
                Output decoded_out = Output::from_memory(decoded_buffer);
                C compressor {*env_p};
                compressor.decompress(text_in, decoded_out);
            }
            std::string decoded {
                decoded_buffer.begin(),
                decoded_buffer.end(),
            };
            ASSERT_EQ(orginal_text, decoded);
        }
    };

    template<class C>
    CompressResult<C> compress(std::string text, Env&& env = Env()) {
        std::unique_ptr<Env> env_p { new Env(std::move(env)) };

        std::vector<uint8_t> encoded_buffer;
        {
            Input text_in = Input::from_memory(text);
            Output encoded_out = Output::from_memory(encoded_buffer);
            C compressor {*env_p};
            compressor.compress(text_in, encoded_out);
        }
        return CompressResult<C> {
            encoded_buffer,
            std::string(encoded_buffer.begin(), encoded_buffer.end()),
            text,
            std::move(env_p),
        };
    }

}

#endif
