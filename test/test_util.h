#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "tudocomp.h"
#include "lz77rule.h"

using namespace tudocomp;
using namespace lz77rule;

// TODO: Actually specialize the 3 kinds

/// Error diagnostic optimized for string data
template<class T, class U>
void assert_eq_strings(const T& expected_, const U& actual_) {
    std::string expected(expected_.begin(), expected_.end());
    std::string actual(actual_.begin(), actual_.end());

    ASSERT_EQ(expected, actual);
}

/// Error diagnostic optimized for binary data
template<class T, class U>
void assert_eq_integers(const T& expected_, const U& actual_) {
    std::vector<uint64_t> expected(expected_.begin(), expected_.end());
    std::vector<uint64_t> actual(actual_.begin(), actual_.end());

    ASSERT_EQ(expected, actual);
}

/// Error diagnostic optimized for mixed binary/ascii data
template<class T, class U>
void assert_eq_hybrid_strings(const T& expected, const U& actual) {
    ASSERT_EQ(expected, actual);
}

/// Error diagnostic optimized for arbitrary data
template<class T, class U>
void assert_eq_sequence(const T& expected, const U& actual) {
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); i++)
        ASSERT_EQ(expected[i], actual[i]);
}

/// A `streambuf` that can be used as a `istream` that points into the
/// contents of another string.
///
/// This is useful for testing the implementation of Coder::decode()
/// with small hardcoded strings.
// DEPRECATED tudocomp offers ViewStream now
struct StringRefStream: std::streambuf
{
    inline StringRefStream(const boost::string_ref vec) {
        this->setg((char*)vec.data(), (char*)vec.data(), (char*)vec.data() + vec.size());
    }
};

/// Temporary provides a `ostream` to write into, and returns it as a string.
///
/// This is useful for testing Coder::code() and Coder::decode().
///
/// \param f A callable type (like for example a C++ lambda expression)
///          that receives an std::ostream& as an argument so that its
///          body can write into it.
template<class Lambda>
std::string ostream_to_string(Lambda f) {
    std::stringstream ss;
    std::ostream& os = ss;
    f(os);
    return ss.str();
}

/// Temporary provides a `ostream` to write into, and returns it as a
/// byte vector.
///
/// This is useful for testing Coder::code() and Coder::decode().
///
/// \param f A callable type (like for example a C++ lambda expression)
///          that receives an std::ostream& as an argument so that its
///          body can write into it.
template<class Lambda>
std::vector<uint8_t> ostream_to_bytes(Lambda f) {
    auto s = ostream_to_string(f);
    return std::vector<uint8_t>(s.begin(), s.end());
}

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

template<class Comp, class Cod>
void lz77roundtrip(const std::string input_string) {
    using R = Rules;

    Env env;

    Comp compressor { env };
    Cod coder { env };

    std::vector<uint8_t> inp_vec { input_string.begin(), input_string.end() };
    Input input = Input::from_memory(inp_vec);

    DLOG(INFO) << "ROUNDTRIP TEXT: " << input_string;

    // Compress to rules
    R rules = compressor.compress(input,
                                coder.min_encoded_rule_length(input.size()));

    DLOG(INFO) << "ROUNDTRIP PRE RULES";

    for (auto e : rules) {
        DLOG(INFO) << "ROUNDTRIP RULE: " << e;
    }

    DLOG(INFO) << "ROUNDTRIP POST RULES";

    // Encode input with rules
    std::string coded_string = ostream_to_string([&] (std::ostream& out_) {
        Output out = Output::from_stream(out_);
        coder.code(std::move(rules), input, out);
    });

    DLOG(INFO) << "ROUNDTRIP CODED: " << vec_to_debug_string(coded_string);

    //Decode again
    std::vector<uint8_t> coded_string_vec {
        coded_string.begin(),
        coded_string.end()
    };
    std::string decoded_string = ostream_to_string([&] (std::ostream& out_) {
        Output out = Output::from_stream(out_);
        Input coded_inp = Input::from_memory(coded_string_vec);

        coder.decode(coded_inp, out);
    });

    DLOG(INFO) << "ROUNDTRIP DECODED: " << decoded_string;

    assert_eq_strings(input_string, decoded_string);
}

template<class F>
void test_roundtrip_batch(F f) {
    f("abcdebcdeabc");
    f("");
    f("abcdebcdeabcd");
    f("a");
    f("foobar");
    f("abcabcabcabc");

    f(
        "asdfasctjkcbweasbebvtiwetwcnbwbbqnqxernqzezwuqwezuet"
        "qcrnzxbneqebwcbqwicbqcbtnqweqxcbwuexcbzqwezcqbwecqbw"
        "dassdasdfzdfgfsdfsdgfducezctzqwebctuiqwiiqcbnzcebzqc");

    f("ประเทศไทย中华Việt Nam");

    f(
        "Lorem ipsum dolor sit amet, sea ut etiam solet salut"
        "andi, sint complectitur et his, ad salutandi imperdi"
        "et gubergren per mei.");

    f(
        "Лорэм атоморюм ут хаж, эа граэки емпыдит ёудёкабет "
        "мэль, декам дежпютатионй про ты. Нэ ёужто жэмпэр"
        " жкрибэнтур векж, незл коррюмпит.");

    f(
        "報チ申猛あち涙境ワセ周兵いわ郵入せすをだ漏告されて話巡わッき"
        "や間紙あいきり諤止テヘエラ鳥提フ健2銀稿97傷エ映田ヒマ役請多"
        "暫械ゅにうて。関国ヘフヲオ場三をおか小都供セクヲ前俳著ゅ向深"
        "まも月10言スひす胆集ヌヱナ賀提63劇とやぽ生牟56詰ひめつそ総愛"
        "ス院攻せいまて報当アラノ日府ラのがし。");

    f(
        "Εαμ ανσιλλαε περισυλα συαφιθαθε εξ, δυο ιδ ρεβυμ σομ"
        "μοδο. Φυγιθ ηομερω ιυς ατ, ει αυδιρε ινθελλεγαμ νες."
        " Ρεκυε ωμνιυμ μανδαμυς κυο εα. Αδμοδυμ σωνσεκυαθ υθ "
        "φιξ, εσθ ετ πρωβατυς συαφιθαθε ραθιονιβυς, ταντας αυ"
        "διαμ ινστρυσθιορ ει σεα.");

    f("struct Foo { uint8_t bar }");

    f("ABBCBCABA");

    f("abcabca");
}

#endif
