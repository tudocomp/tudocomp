#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "tudocomp.h"

using namespace tudocomp;

template<class Comp, class Cod>
void test_roundtrip(const std::string input_string) {
    Comp compressor;
    Cod coder;

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
