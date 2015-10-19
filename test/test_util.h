#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "tudocomp.h"

using namespace tudocomp;

/// Create a `Input` value containing `s`.
///
/// This is useful for testing the implementations of
/// Compressor::Compress() and Coder::code() with small hardcoded strings.
inline Input input_from_string(std::string s) {
    Input v(s.begin(), s.end());
    return v;
}

/// A `streambuf` that can be used as a `istream` that points into the
/// contents of another string.
///
/// This is useful for testing the implementation of Coder::decode()
/// with small hardcoded strings.
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
    Input m_input;
    Rules m_expected_rules;
    int m_threshold = 2;

    inline CompressorTest input(std::string inp) {
        CompressorTest copy = *this;
        copy.m_input = input_from_string(inp);
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
        Rules rules = compressor.compress(m_input, m_threshold);

        ASSERT_EQ(rules.size(), m_expected_rules.size());
        for (size_t i = 0; i < rules.size(); i++)
            ASSERT_EQ(rules[i], m_expected_rules[i]);
    }
};

/// A simple builder class for testing the implementation of a encoder.
template<class T>
struct CoderTest {
    Env env;
    T coder { env };
    Input m_input;
    Rules m_rules;
    std::string m_expected_output;
    bool output_is_string = true;

    inline CoderTest input(std::string inp) {
        CoderTest copy = *this;
        copy.m_input = input_from_string(inp);
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
        if (output_is_string) {
            auto actual_output = ostream_to_string([&] (std::ostream& out) {
                coder.code(m_rules, m_input, out);
            });
            ASSERT_EQ(m_expected_output, actual_output);
        } else {
            auto actual_output = ostream_to_bytes([&] (std::ostream& out) {
                coder.code(m_rules, m_input, out);
            });
            ASSERT_EQ(std::vector<uint8_t>(
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
    std::string m_input;
    std::string m_expected_output;

    inline DecoderTest input(std::string inp) {
        DecoderTest copy = *this;
        copy.m_input = inp;
        return copy;
    }

    inline DecoderTest input(std::vector<uint8_t> inp) {
        DecoderTest copy = *this;
        copy.m_input = std::string(inp.begin(), inp.end());
        return copy;
    }

    inline DecoderTest expected_output(std::string out) {
        DecoderTest copy = *this;
        copy.m_expected_output = out;
        return copy;
    }

    inline void run() {
        std::istringstream input_stream(m_input);

        auto actual_output = ostream_to_string([&] (std::ostream& out) {
            decoder.decode(input_stream, out);
        });
        ASSERT_EQ(m_expected_output, actual_output);
    }
};

template<class Comp, class Cod>
void test_roundtrip(const std::string input_string) {
    Env env;

    Comp compressor { env };
    Cod coder { env };

    const Input input = input_from_string(input_string);

    DLOG(INFO) << "ROUNDTRIP TEXT: " << input_string;

    // Compress to rules
    Rules rules = compressor.compress(input,
                                      coder.min_encoded_rule_length(input.size()));

    DLOG(INFO) << "ROUNDTRIP PRE RULES";

    for (Rule rule : rules) {
        //DLOG(INFO) << "ROUNDTRIP PRE RULE";
        DLOG(INFO) << "ROUNDTRIP RULE: " << rule;
    }

    DLOG(INFO) << "ROUNDTRIP POST RULES";

    // Encode input with rules
    std::string coded_string = ostream_to_string([&] (std::ostream& out) {
        coder.code(rules, input, out);
    });

    DLOG(INFO) << "ROUNDTRIP CODED: " << vec_to_debug_string(coded_string);

    //Decode again
    std::istringstream coded_stream(coded_string);
    std::string decoded_string = ostream_to_string([&] (std::ostream& out) {
        coder.decode(coded_stream, out);
    });

    DLOG(INFO) << "ROUNDTRIP DECODED: " << decoded_string;

    ASSERT_EQ(input_string, decoded_string);
}

template<class T, class U>
void test_roundtrip_batch() {
    test_roundtrip<T, U>("abcdebcdeabc");
    test_roundtrip<T, U>("");
    test_roundtrip<T, U>("abcdebcdeabcd");
    test_roundtrip<T, U>("a");
    test_roundtrip<T, U>("foobar");
    test_roundtrip<T, U>("abcabcabcabc");

    test_roundtrip<T, U>("asdfasctjkcbweasbebvtiwetwcnbwbbqnqxernqzezwuqwezuet"
                         "qcrnzxbneqebwcbqwicbqcbtnqweqxcbwuexcbzqwezcqbwecqbw"
                         "dassdasdfzdfgfsdfsdgfducezctzqwebctuiqwiiqcbnzcebzqc");

    test_roundtrip<T, U>("ประเทศไทย中华Việt Nam");

    test_roundtrip<T, U>("Lorem ipsum dolor sit amet, sea ut etiam solet salut"
                         "andi, sint complectitur et his, ad salutandi imperdi"
                         "et gubergren per mei.");

    test_roundtrip<T, U>("Лорэм атоморюм ут хаж, эа граэки емпыдит ёудёкабет "
                         "мэль, декам дежпютатионй про ты. Нэ ёужто жэмпэр"
                         " жкрибэнтур векж, незл коррюмпит.");

    test_roundtrip<T, U>("報チ申猛あち涙境ワセ周兵いわ郵入せすをだ漏告されて話巡わッき"
                         "や間紙あいきり諤止テヘエラ鳥提フ健2銀稿97傷エ映田ヒマ役請多"
                         "暫械ゅにうて。関国ヘフヲオ場三をおか小都供セクヲ前俳著ゅ向深"
                         "まも月10言スひす胆集ヌヱナ賀提63劇とやぽ生牟56詰ひめつそ総愛"
                         "ス院攻せいまて報当アラノ日府ラのがし。");

    test_roundtrip<T, U>("Εαμ ανσιλλαε περισυλα συαφιθαθε εξ, δυο ιδ ρεβυμ σομ"
                         "μοδο. Φυγιθ ηομερω ιυς ατ, ει αυδιρε ινθελλεγαμ νες."
                         " Ρεκυε ωμνιυμ μανδαμυς κυο εα. Αδμοδυμ σωνσεκυαθ υθ "
                         "φιξ, εσθ ετ πρωβατυς συαφιθαθε ραθιονιβυς, ταντας αυ"
                         "διαμ ινστρυσθιορ ει σεα.");

    test_roundtrip<T, U>("struct Foo { uint8_t bar }");
}

#endif
