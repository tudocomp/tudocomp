#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "tudocomp.h"

using namespace tudocomp;

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
