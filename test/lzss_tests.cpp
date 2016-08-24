/*
    These tests are merely to support development and
    should not be seen as actual tests.

    However, they should likely be transformed into actual
    tests at some point. See this as a staging area
    for new features.
*/

#include <gtest/gtest.h>

#include <glog/logging.h>

#include <tudocomp/io.h>

#include <tudocomp/lzss/LZ77SSLCPCompressor.hpp>
#include <tudocomp/lzss/LZ77SSSlidingWindowCompressor.hpp>
#include <tudocomp/lzss/LZSSESACompressor.hpp>

#include <tudocomp/lzss/DebugLZSSCoder.hpp>
#include <tudocomp/lzss/OfflineLZSSCoder.hpp>
#include <tudocomp/lzss/OnlineLZSSCoder.hpp>

#include <tudocomp/alphabet/OfflineAlphabetCoder.hpp>
#include <tudocomp/alphabet/OnlineAlphabetCoder.hpp>

using namespace tudocomp;

const std::string input_str = "abcdefgh#defgh_abcde";

std::string hex_bytes_str(const std::string& str) {
    std::stringstream result;
    for(size_t i = 0; i < str.length(); i++) {
        result << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << uint32_t((uint8_t(str[i]))) << " ";
    }
    return result.str();
}

template<typename C>
void performTest() {
    DLOG(INFO) << "Input: " << input_str;

    //compress
    std::string comp_result;
    {
        Input input = Input::from_memory(input_str);

        std::stringstream stm;
        Output output = Output::from_stream(stm);

        auto compressor = create_algo<C>();
        compressor.compress(input, output);

        comp_result = stm.str();
    }

    char c = comp_result[0];
    DLOG(INFO) << "Compression result: " << ((c >= '0' && c <= '9') ? comp_result : hex_bytes_str(comp_result));

    //decompress
    std::string decomp_result;
    {
        Input input = Input::from_memory(comp_result);

        std::stringstream stm;
        Output output = Output::from_stream(stm);

        auto compressor = create_algo<C>();
        compressor.decompress(input, output);

        decomp_result = stm.str();
    }

    DLOG(INFO) << "Decompression result: " << decomp_result;

    ASSERT_EQ(input_str, decomp_result);
}

TEST(CodingPrototype, lz77ss_sw_debug) {
    performTest<lzss::LZ77SSSlidingWindowCompressor<lzss::DebugLZSSCoder>>();
}

TEST(CodingPrototype, lz77ss_sw_Aonline_Fonline) {
    performTest<lzss::LZ77SSSlidingWindowCompressor<lzss::OnlineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lz77ss_sw_Aoffline_Fonline) {
    performTest<lzss::LZ77SSSlidingWindowCompressor<lzss::OnlineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lz77ss_sw_Aonline_Foffline) {
    performTest<lzss::LZ77SSSlidingWindowCompressor<lzss::OfflineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lz77ss_sw_Aoffline_Foffline) {
    performTest<lzss::LZ77SSSlidingWindowCompressor<lzss::OfflineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lz77ss_lcp_debug) {
    performTest<lzss::LZ77SSLCPCompressor<lzss::DebugLZSSCoder>>();
}

TEST(CodingPrototype, lz77ss_lcp_Aonline_Fonline) {
    performTest<lzss::LZ77SSLCPCompressor<lzss::OnlineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lz77ss_lcp_Aoffline_Fonline) {
    performTest<lzss::LZ77SSLCPCompressor<lzss::OnlineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lz77ss_lcp_Aonline_Foffline) {
    performTest<lzss::LZ77SSLCPCompressor<lzss::OfflineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lz77ss_lcp_Aoffline_Foffline) {
    performTest<lzss::LZ77SSLCPCompressor<lzss::OfflineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_maxlcp_debug) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompMaxLCP, lzss::DebugLZSSCoder>>();
}

TEST(CodingPrototype, lzss_esacomp_maxlcp_Aonline_Fonline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompMaxLCP, lzss::OnlineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_maxlcp_Aoffline_Fonline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompMaxLCP, lzss::OnlineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_maxlcp_Aonline_Foffline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompMaxLCP, lzss::OfflineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_maxlcp_Aoffline_Foffline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompMaxLCP, lzss::OfflineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_bulldozer_debug) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompBulldozer, lzss::DebugLZSSCoder>>();
}

TEST(CodingPrototype, lzss_esacomp_bulldozer_Aonline_Fonline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompBulldozer, lzss::OnlineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_bulldozer_Aoffline_Fonline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompBulldozer, lzss::OnlineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_bulldozer_Aonline_Foffline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompBulldozer, lzss::OfflineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_bulldozer_Aoffline_Foffline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompBulldozer, lzss::OfflineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_linear_debug) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompLinear, lzss::DebugLZSSCoder>>();
}

TEST(CodingPrototype, lzss_esacomp_linear_Aonline_Fonline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompLinear, lzss::OnlineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_linear_Aoffline_Fonline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompLinear, lzss::OnlineLZSSCoder<OfflineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_linear_Aonline_Foffline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompLinear, lzss::OfflineLZSSCoder<OnlineAlphabetCoder>>>();
}

TEST(CodingPrototype, lzss_esacomp_linear_Aoffline_Foffline) {
    performTest<lzss::LZSSESACompressor<lzss::ESACompLinear, lzss::OfflineLZSSCoder<OfflineAlphabetCoder>>>();
}
