#include <cmath>
#include <algorithm>
#include <utility>

#include "boost/utility/string_ref.hpp"
#include "boost/functional/hash.hpp"
#include "glog/logging.h"

#include "code2.h"
#include "bit_iostream.h"
#include "counter.h"

// TODO
namespace std
{
    template<>
    struct hash<boost::string_ref> {
        size_t operator()(boost::string_ref const& sr) const {
            return boost::hash_range(sr.begin(), sr.end());
        }
    };
}

namespace esacomp {

using OutputSize = uint64_t;
using ThresholdSize = uint32_t;
using BitPerSymbolSize = uint8_t;
using BitPerSubLenSize = uint8_t;
using BitPerRefSize = uint8_t;

// 8 for bytes + 8 for phrases
using AlphabetSize = uint16_t;
// TODO: (Could be 0x100, but 0xf800 matches the java output)
const uint16_t PHRASE_ID = 0xf800;
const size_t KGRAM = 3;

static size_t bitsFor(size_t n) {
    // TODO: Maybe use nice bit ops?
    return size_t(ceil(log2(std::max(size_t(2), n))));
}

static void code2Raw(BitOstream& out, size_t c, size_t maxBits) {
    if (maxBits > 6) {
        if (c < 4) {
            out.write(0b000, 3);
            out.write(c, 2);
        } else if (c < 8) {
            out.write(0b001, 3);
            out.write(c - 4, 2);
        } else if (c < 12) {
            out.write(0b010, 3);
            out.write(c - 8, 2);
        } else if (c < 16) {
            out.write(0b011, 3);
            out.write(c - 12, 2);
        } else if (c < 24) {
            out.write(0b100, 3);
            out.write(c - 16, 3);
        } else if (c < 32) {
            out.write(0b101, 3);
            out.write(c - 24, 3);
        } else if (c < 40) {
            out.write(0b110, 3);
            out.write(c - 32, 3);
        } else {
            out.write(0b111, 3);
            out.write(c, maxBits);
        }
    } else if (maxBits > 5) {
        if (c < 8) {
            out.write(0b00, 2);
            out.write(c, 3);
        } else if (c < 16) {
            out.write(0b01, 2);
            out.write(c - 8, 3);
        } else if (c < 32) {
            out.write(0b10, 2);
            out.write(c - 16, 4);
        } else {
            out.write(0b11, 2);
            out.write(c, maxBits);
        }
    } else if (maxBits > 3) {
        out.writeBit(c < 4);
        out.write(c, (c < 4) ? 2 : maxBits);
    } else {
        out.write(c, maxBits);
    }
}

static void code2Ref(BitOstream& out, size_t ref, size_t maxBits) {
    out.write<size_t>(ref, maxBits);
}

static void code2Length(BitOstream& out, size_t n, size_t maxBits) {
    if (n < 8) {
        out.write(0b00, 2);
        out.write(n, 3);
    } else if (n < 16) {
        out.write(0b01, 2);
        out.write(n - 8, 3);
    } else if (n < 32) {
        out.write(0b10, 2);
        out.write(n - 16, 4);
    } else {
        out.write(0b11, 2);
        out.write(n, maxBits);
    }
}

void Code2Coder::code(Rules rules, Input input, std::ostream& out_) {
    ////////////////////////////
    // original Java Code constructor

    ssize_t longestSubstitution = 0;
    ssize_t largestRefAbs = 0;

    // Threshold, aka minimum rule size.
    // Used such that the actual minimum rule size
    // can be offset to start at 0.
    size_t threshold = SIZE_MAX;
    {
        for (Rule rule: rules) {
            longestSubstitution = std::max(longestSubstitution, ssize_t(rule.num));
            largestRefAbs = std::max(largestRefAbs, ssize_t(rule.source));
            threshold = std::min(threshold, rule.num);
        }

        if (threshold == SIZE_MAX) {
            threshold = 0;
        }
    }

    ////////////////////////////
    // original Java code2() method

    BitOstream out(out_);

    // Create a lightweight string_ref to the input data.
    boost::string_ref text((char*) input.data(), input.size());

    // TODO: Make configurable with 3 as default
    size_t kgram = KGRAM;

    // alphabet: 8bit for all possible bytes, plus room for artifical new symbols
    // => 16 bit needed.
    Counter<AlphabetSize> alphabet;
    Counter<boost::string_ref> phrases;

    DLOG(INFO) << "collect phrases [";

    size_t p = 0;
    for (Rule rule : rules) {
        for (; p < rule.target; p++) {
            alphabet.increase((unsigned char)text[p]);

            if (p + kgram < rule.target) {
                auto s = text.substr(p, kgram);
                phrases.increase(s);
                DLOG(INFO) << "  " << s << " " << phrases.getCount(s);
            }
        }
        p = rule.target + rule.num;
    }

    for (; p < text.size(); p++) {
        alphabet.increase((unsigned char)text[p]);

        if (p + kgram < text.size()) {
            auto s = text.substr(p, kgram);
            phrases.increase(s);
            DLOG(INFO) << "  " << s << " " << phrases.getCount(s);
        }
    }

    DLOG(INFO) << "]";

    //calculate alpha
    size_t alpha;
    // TODO: alpha getProperty
    /*ELSE*/ {
        auto numItems = alphabet.getNumItems();

        // TODO: Ensure correct behavior for numitems == 0 or numitems == 1
        size_t bits = bitsFor(numItems);
        size_t addBits = (size_t(1 << bits) == numItems) ? 1 : 2;

        alpha = (1 << (bits + addBits)) - numItems;
    }

    //find alphabet facts and count phrases of length k
    std::unordered_map<boost::string_ref, size_t> phraseRanking;
    if (kgram > 1) {
        phraseRanking = phrases.createRanking(alpha);
    }

    //introduce a new symbol for each phrase
    if (kgram > 1) {
        for (auto pair : phraseRanking) {
            alphabet.setCount(PHRASE_ID + pair.second, phrases.getCount(pair.first));
        }
    }

    std::unordered_map<AlphabetSize, size_t> alphabetRanking = alphabet.createRanking();

    size_t alphabetSize = alphabet.getNumItems();
    // TODO: Verify that the use of bitsFor is correct here
    size_t bitsPerRef = bitsFor(largestRefAbs);
    size_t bitsPerSubLen = bitsFor(longestSubstitution);
    size_t bitsPerSymbol = bitsFor(alphabetSize);

    DLOG(INFO) << "alphabet ranking [";
    for (auto pair : alphabetRanking) {
        auto chr = pair.first;
        if (chr < 255) {
            DLOG(INFO) << "  " << char(chr) << ", " << pair.second;
        } else {
            DLOG(INFO) << "  PHRASE_ID + " << chr - PHRASE_ID << ", " << pair.second;
        }
    }
    DLOG(INFO) << "]";

    out.write<OutputSize>(text.size());
    DLOG(INFO) << "write OutputSize " << OutputSize(text.size());
    out.write<ThresholdSize>(threshold);
    out.write<BitPerSymbolSize>(bitsPerSymbol);
    out.write<BitPerSubLenSize>(bitsPerSubLen);
    out.write<BitPerRefSize>(bitsPerRef);

    // write alphabet in ranking order
    out.write<AlphabetSize>(alphabet.getNumItems());
    for (auto pair : alphabet.getSorted()) {
        out.write<AlphabetSize>(pair.first);
    }

    // write k-grams in ranking order
    out.write<AlphabetSize>(phraseRanking.size());
    size_t i = 0;
    for (auto pair : phrases.getSorted()) {
        if (++i > phraseRanking.size()) {
            break;
        }

        out.write_aligned_bytes(pair.first.data(), pair.first.size());
    }

    DLOG(INFO) << "phrase ranking [";
    for(auto pair : phraseRanking) {
        DLOG(INFO) << "  " << pair.first << " " << pair.second;
    }
    DLOG(INFO) << "]";

    // bit stream starts here

    p = 0;
    boost::string_ref phrase;

    auto encode_raw_until = [&] (size_t up_to) {
        for (; p < up_to; p++) {
            out.writeBit(0);
            size_t c;
            phrase = text.substr(p, kgram);
            if (kgram > 1
                && p + kgram < up_to
                && phraseRanking.count(phrase) > 0)
            {
                size_t tmp = PHRASE_ID + phraseRanking[phrase];
                c = alphabetRanking[tmp];
                p += kgram - 1;
            } else {
                c = alphabetRanking[(unsigned char)text[p]];
            }
            code2Raw(out, c, bitsPerSymbol);
        }
    };

    // TODO: Enforce need for sorted Rules somehow
    for (Rule rule : rules) {
        // encode all chars until start of rule
        encode_raw_until(rule.target);
        p = rule.target + rule.num;

        // encode rule
        out.writeBit(1);
        code2Ref(out, rule.source, bitsPerRef); //absolute reference
        code2Length(out, rule.num - threshold, bitsPerSubLen);
    }
    encode_raw_until(text.size());

    out.flush();
}

static AlphabetSize codeFromRaw(BitIstream& inp, size_t maxBits) {
    if (maxBits > 6) {
        uint8_t tag = inp.readBits<uint8_t>(3);

        switch(tag) {
        case 0b000:
            return inp.readBits<AlphabetSize>(2);
        case 0b001:
            return inp.readBits<AlphabetSize>(2) + 4;
        case 0b010:
            return inp.readBits<AlphabetSize>(2) + 8;
        case 0b011:
            return inp.readBits<AlphabetSize>(2) + 12;
        case 0b100:
            return inp.readBits<AlphabetSize>(3) + 16;
        case 0b101:
            return inp.readBits<AlphabetSize>(3) + 24;
        case 0b110:
            return inp.readBits<AlphabetSize>(3) + 32;
        default:
            return inp.readBits<AlphabetSize>(maxBits);
        }
    } else if (maxBits > 5) {
        uint8_t tag = inp.readBits<uint8_t>(2);

        switch(tag) {
        case 0b00:
            return inp.readBits<AlphabetSize>(3);
        case 0b01:
            return inp.readBits<AlphabetSize>(3) + 8;
        case 0b10:
            return inp.readBits<AlphabetSize>(4) + 16;
        default:
            return inp.readBits<AlphabetSize>(maxBits);
        }
    } else if (maxBits > 3) {
        uint8_t tag = inp.readBits<uint8_t>(1);

        switch(tag) {
        case 0b1:
            return inp.readBits<AlphabetSize>(2);
        default:
            return inp.readBits<AlphabetSize>(maxBits);
        }
    } else {
        return inp.readBits<AlphabetSize>(maxBits);
    }
}

static size_t codeFromRef(BitIstream& inp, size_t maxBits) {
    return inp.readBits<size_t>(maxBits);
}

static size_t codeFromLength(BitIstream& inp, size_t maxBits) {
    uint8_t tag = inp.readBits<uint8_t>(2);

    switch (tag) {
    case 0b00:
        return inp.readBits<uint8_t>(3);
    case 0b01:
        return inp.readBits<uint8_t>(3) + 8;
    case 0b10:
        return inp.readBits<uint8_t>(4) + 16;
    default:
        // TODO: Use of size_t here is not entirely portable
        return inp.readBits<size_t>(maxBits);
    }
}

void Code2Coder::decode(std::istream& inp, std::ostream& out) {
    // read byte encoded header

    OutputSize length = read_bytes<OutputSize>(inp);
    DLOG(INFO) << "text length: " << length;

    ThresholdSize threshold = read_bytes<ThresholdSize>(inp);
    DLOG(INFO) << "threshold length: " << threshold;

    BitPerSymbolSize bitsPerSymbol = read_bytes<BitPerSymbolSize>(inp);
    DLOG(INFO) << "bitsPerSymbol: " << int(bitsPerSymbol);

    BitPerSubLenSize bitsPerSubLen = read_bytes<BitPerSubLenSize>(inp);
    DLOG(INFO) << "bitsPerSubLen: " << int(bitsPerSubLen);

    BitPerRefSize bitsPerRef = read_bytes<BitPerRefSize>(inp);
    DLOG(INFO) << "bitsPerRef: " << int(bitsPerRef);

    AlphabetSize alphabet = read_bytes<AlphabetSize>(inp);
    DLOG(INFO) << "alphabetSize: " << alphabet;

    std::unordered_map<AlphabetSize, AlphabetSize> index_to_symbol;
    for (size_t i = 0; i < alphabet; i++) {
        AlphabetSize c = read_bytes<AlphabetSize>(inp);
        index_to_symbol[i] = c;
        DLOG(INFO) << "symbol["<<i<<"]: " << index_to_symbol[i];
    }

    AlphabetSize phrases = read_bytes<AlphabetSize>(inp);
    DLOG(INFO) << "phrases: " << phrases;

    size_t phrase_len = KGRAM;

    std::vector<uint8_t> phrase_buffer;
    phrase_buffer.reserve(phrases * phrase_len);

    for (size_t i = 0; i < phrases; i++) {
        read_bytes_to_vec(inp, phrase_buffer, phrase_len);
        boost::string_ref x((const char*) phrase_buffer.data()
                            + i * phrase_len,
                            phrase_len);
        DLOG(INFO) << "phrase["<<i<<"]: " << x;
    }

    // read bit encoded text;
    BitIstream binp(inp);
    DecodeBuffer buffer(length);
    DLOG(INFO) << "building text:";

     while (buffer.size() < length) {
        uint8_t bit = binp.readBit();
        if (bit == 0) {
            // read a extended char
            AlphabetSize char_index = codeFromRaw(binp, bitsPerSymbol);
            DLOG(INFO) << "char_index: " << char_index;

            AlphabetSize extended_char = index_to_symbol[char_index];
            if (extended_char >= PHRASE_ID) {
                AlphabetSize phrase_index = extended_char - PHRASE_ID;

                for(size_t i = 0; i < KGRAM; i++) {
                    uint8_t byte = phrase_buffer[phrase_index * KGRAM + i];
                    buffer.push_decoded_byte(byte);
                }
                DLOG(INFO) << "ok";
            } else {
                buffer.push_decoded_byte(extended_char);
            }
        } else {
            // read a rule
            size_t source = codeFromRef(binp, bitsPerRef);
            size_t num = codeFromLength(binp, bitsPerSubLen) + threshold;
            DLOG(INFO) << "source: " << source;
            DLOG(INFO) << "num: " << num;
            buffer.push_rule(source, num);
        }
    }

    buffer.write_to(out);
}

size_t Code2Coder::min_encoded_rule_length(size_t input_size) {
    // TODO: Think about whether this is right
    return 2;
}

}
