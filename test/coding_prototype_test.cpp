#include <gtest/gtest.h>

#include <glog/logging.h>

#include <tudocomp/io.h>

#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/lzss/LZ77SSFactorizer.hpp>
#include <tudocomp/lzss/OfflineLZSSCoder.hpp>
#include <tudocomp/lzss/OnlineLZSSCoder.hpp>

#include <tudocomp/proto/OfflineAlphabetCoder.hpp>
#include <tudocomp/proto/OnlineAlphabetCoder.hpp>

#include <tudocomp/proto/Compressor.hpp>

const std::string input_str = "aaaaaaaaaaaa";

std::string hex_bytes_str(const std::string& str) {
    std::stringstream result;
    for(size_t i = 0; i < str.length(); i++) {
        result << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << uint32_t((uint8_t(str[i]))) << " ";
    }
    return result.str();
}

TEST(CodingPrototype, lz77ss_Aonline_Fonline) {
    using namespace tudocomp;

    DLOG(INFO) << "Input: " << input_str;

    Env env;
    Input input = Input::from_memory(input_str);
    
    std::stringstream stm;
    Output output = Output::from_stream(stm);
    
    factor_compress<lzss::LZ77SSFactorizer, OnlineAlphabetCoder, lzss::OnlineLZSSCoder>(env, input, output);
    
    DLOG(INFO) << "Result: " << hex_bytes_str(stm.str());
}

TEST(CodingPrototype, lz77ss_Aoffline_Fonline) {
    using namespace tudocomp;

    DLOG(INFO) << "Input: " << input_str;

    Env env;
    Input input = Input::from_memory(input_str);
    
    std::stringstream stm;
    Output output = Output::from_stream(stm);
    
    factor_compress<lzss::LZ77SSFactorizer, OfflineAlphabetCoder, lzss::OnlineLZSSCoder>(env, input, output);
    
    DLOG(INFO) << "Result: " << hex_bytes_str(stm.str());
}

TEST(CodingPrototype, lz77ss_Aonline_Foffline) {
    using namespace tudocomp;

    DLOG(INFO) << "Input: " << input_str;

    Env env;
    Input input = Input::from_memory(input_str);
    
    std::stringstream stm;
    Output output = Output::from_stream(stm);
    
    factor_compress<lzss::LZ77SSFactorizer, OnlineAlphabetCoder, lzss::OfflineLZSSCoder>(env, input, output);
    
    DLOG(INFO) << "Result: " << hex_bytes_str(stm.str());
}

TEST(CodingPrototype, lz77ss_Aoffline_Foffline) {
    using namespace tudocomp;

    DLOG(INFO) << "Input: " << input_str;

    Env env;
    Input input = Input::from_memory(input_str);
    
    std::stringstream stm;
    Output output = Output::from_stream(stm);
    
    factor_compress<lzss::LZ77SSFactorizer, OfflineAlphabetCoder, lzss::OfflineLZSSCoder>(env, input, output);
    
    DLOG(INFO) << "Result: " << hex_bytes_str(stm.str());
}

