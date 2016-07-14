#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <cstdint>
#include <fstream>
#include <iostream>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <sys/stat.h>

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

/// Call the given function with a number
/// of different strings testing common corner cases and unicode input.
template<class F>
void test_roundtrip_batch(F f) {
    f("abcdebcdeabc");
    f("a");
    f("");
    f("abcdebcdeabcd");
    f("foobar");
    f("abcabcabcabc");

    f("abc abc  abc");

    f("abaaabbababb");

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

    f("abbbbbbbbbbcbbbbbbbbbb");
}

const std::string TEST_FILE_PATH = "test_files";

inline std::string test_file_path(const std::string& filename) {
    return TEST_FILE_PATH + "/" + filename;
}

inline bool test_file_exists(const std::string& filename) {
    std::string test_file_name = test_file_path(filename);

    struct stat buf;
    return (stat(test_file_name.c_str(), &buf) == 0);
}

inline std::string read_test_file(const std::string& filename) {
    std::ostringstream sout;

    std::string test_file_name = test_file_path(filename);
    std::ifstream fin(test_file_name);
    if(fin) {
        std::copy(std::istreambuf_iterator<char>(fin),
              std::istreambuf_iterator<char>(),
              std::ostreambuf_iterator<char>(sout));

        fin.close();
    } else {
        std::string msg = "Could not open test file \"";
        msg += test_file_name;
        msg += "\"";
        throw std::runtime_error(msg);
    }
    return sout.str();
}

inline void create_test_directory() {
    mkdir(TEST_FILE_PATH.c_str(), 0777);
}

inline void write_test_file(const std::string& filename, const std::string& text) {
    create_test_directory();
    std::ofstream fout(test_file_path(filename));
    if(fout) {
        fout << text;
        fout.close();
    }
}

inline void remove_test_file(const std::string& filename) {
    create_test_directory();
    remove(test_file_path(filename).c_str());
}

inline std::vector<uint8_t> pack_integers(std::vector<uint64_t> ints) {
    CHECK(ints.size() % 2 == 0);
    std::vector<uint8_t> bits;

    uint bit_pos = 8;
    for (size_t i = 0; i < ints.size(); i += 2) {
        uint64_t val = ints[i];
        uint64_t val_bits = ints[i + 1];
        for (uint64_t bit = 0; bit < val_bits; bit++) {
            if (bit_pos == 8) {
                bits.push_back(0);
                bit_pos = 0;
            }

            uint8_t& b = bits[bits.size() - 1];
            if (val & (uint64_t(1) << (val_bits - bit - 1))) {
                b |= (1 << (7 - bit_pos));
            }

            bit_pos++;
        }
    }

    return bits;
}

#endif
