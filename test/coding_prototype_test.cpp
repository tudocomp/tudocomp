#include <gtest/gtest.h>

#include <glog/logging.h>

#include <tudocomp/io.h>
#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/lzss/LZ77SSFactorizer.hpp>
#include <tudocomp/lzss/OfflineLZSSCoder.hpp>
#include <tudocomp/proto/OfflineAlphabetCoder.hpp>
#include <tudocomp/proto/OfflineCompressor.hpp>

TEST(CodingPrototype, offline) {
    using namespace tudocomp;

    Env env;
    
    Input input = Input::from_memory("abaxaba");
    
    std::stringstream stm;
    Output output = Output::from_stream(stm);
    
    OfflineCompressor<lzss::LZ77SSFactorizer, OfflineAlphabetCoder, lzss::OfflineLZSSCoder> c(env);
    c.compress(input, output);
    
    std::string str = stm.str();

    std::stringstream result;
    for(size_t i = 0; i < str.length(); i++) {
        result << std::hex << uint32_t((uint8_t(str[i]))) << " ";
    }
    DLOG(INFO) << "Result: " << result.str();
}
