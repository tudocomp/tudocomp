#ifndef LZ77_RULE_TEST_UTIL_H
#define LZ77_RULE_TEST_UTIL_H

#include <tudocomp/tudocomp.hpp>

namespace lz77rule_test {

using namespace tdc;
using namespace esacomp;

/// A simple builder class for testing the implementation of a compressor.
template<class T>
struct CompressorTest {
    Env env;
    T compressor { env };
    std::vector<uint8_t> m_input;
    Rules m_expected_rules;
    int m_threshold = 2;

    inline CompressorTest input(std::string inp) {
        CompressorTest copy = *this;
        copy.m_input = std::vector<uint8_t> { inp.begin(), inp.end() };
        return copy;
    }

    inline CompressorTest expected_rules(Rules expected) {
        CompressorTest copy = *this;
        copy.m_expected_rules = expected;
        return copy;
    }

    inline CompressorTest threshold(int thresh) {
        CompressorTest copy = *this;
        copy.m_threshold = thresh;
        return copy;
    }

    inline void run() {
        Input inp = Input::from_memory(m_input);
        Rules rules = compressor.compress(inp, m_threshold);

        assert_eq_sequence(rules, m_expected_rules);
    }
};

/// A simple builder class for testing the implementation of a encoder.
template<class T>
struct CoderTest {
    Env env;
    T coder { env };
    std::vector<uint8_t> m_input;
    Rules m_rules;
    std::string m_expected_output;
    bool output_is_string = true;

    inline CoderTest input(std::string inp) {
        CoderTest copy = *this;
        copy.m_input = std::vector<uint8_t> { inp.begin(), inp.end() };
        return copy;
    }

    inline CoderTest rules(Rules rs) {
        CoderTest copy = *this;
        copy.m_rules = rs;
        return copy;
    }

    inline CoderTest expected_output(std::string out) {
        CoderTest copy = *this;
        copy.m_expected_output = out;
        copy.output_is_string = true;
        return copy;
    }

    inline CoderTest expected_output(std::vector<uint8_t> out) {
        CoderTest copy = *this;
        copy.m_expected_output = std::string(out.begin(), out.end());
        copy.output_is_string = false;
        return copy;
    }

    inline void run() {
        Input inp = Input::from_memory(m_input);
        if (output_is_string) {
            auto actual_output = ostream_to_string([&] (std::ostream& out_) {
                Output out = Output::from_stream(out_);
                coder.code(std::move(m_rules), inp, out);
            });
            assert_eq_strings(m_expected_output, actual_output);
        } else {
            auto actual_output = ostream_to_bytes([&] (std::ostream& out_) {
                Output out = Output::from_stream(out_);
                coder.code(std::move(m_rules), inp, out);
            });
            assert_eq_hybrid_strings(std::vector<uint8_t>(
                        m_expected_output.begin(),
                        m_expected_output.end()),
                      actual_output);
        }
    }
};

/// A simple builder class for testing the implementation of a encoder.
template<class T>
struct DecoderTest {
    Env env;
    T decoder { env };
    std::vector<uint8_t> m_input;
    std::string m_expected_output;

    inline DecoderTest input(std::string inp) {
        DecoderTest copy = *this;
        copy.m_input = std::vector<uint8_t> { inp.begin(), inp.end() };
        return copy;
    }

    inline DecoderTest input(std::vector<uint8_t> inp) {
        DecoderTest copy = *this;
        copy.m_input = inp;
        return copy;
    }

    inline DecoderTest expected_output(std::string out) {
        DecoderTest copy = *this;
        copy.m_expected_output = out;
        return copy;
    }

    inline void run() {
        Input inp = Input::from_memory(m_input);
        auto actual_output = ostream_to_string([&] (std::ostream& out_) {
            Output out = Output::from_stream(out_);
            decoder.decode(inp, out);
        });
        assert_eq_strings(m_expected_output, actual_output);
    }
};

}

#endif
