#include <gtest/gtest.h>

#include <glog/logging.h>

#include <tudocomp/io.h>
#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/lzss/LZ77SSFactorizer.hpp>

#include <tudocomp/lzss/OfflineLZSSCoder.hpp>
#include <tudocomp/proto/OfflineAlphabetCoder.hpp>
#include <tudocomp/proto/OfflineCompressor.hpp>

#include <tudocomp/lzss/OnlineLZSSCoder.hpp>
#include <tudocomp/proto/OnlineAlphabetCoder.hpp>
#include <tudocomp/proto/OnlineCompressor.hpp>

const std::string input_str = "abcXXabcYYabc";

std::string hex_bytes_str(const std::string& str) {
    std::stringstream result;
    for(size_t i = 0; i < str.length(); i++) {
        result << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << uint32_t((uint8_t(str[i]))) << " ";
    }
    return result.str();
}

TEST(CodingPrototype, offline) {
    using namespace tudocomp;

    Env env;
    Input input = Input::from_memory(input_str);
    
    std::stringstream stm;
    Output output = Output::from_stream(stm);
    
    OfflineCompressor<lzss::LZ77SSFactorizer, OfflineAlphabetCoder, lzss::OfflineLZSSCoder> c(env);
    c.compress(input, output);
    
    DLOG(INFO) << "Result: " << hex_bytes_str(stm.str());
}

TEST(CodingPrototype, online) {
    using namespace tudocomp;

    Env env;
    Input input = Input::from_memory(input_str);
    
    std::stringstream stm;
    Output output = Output::from_stream(stm);
    
    OnlineCompressor<lzss::LZ77SSFactorizer, OnlineAlphabetCoder, lzss::OnlineLZSSCoder> c(env);
    c.compress(input, output);
    
    DLOG(INFO) << "Result: " << hex_bytes_str(stm.str());
}
