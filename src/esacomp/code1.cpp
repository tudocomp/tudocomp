#include <cmath>
#include <algorithm>

#include "glog/logging.h"

#include "code1.h"
#include "tudocomp.h"

namespace esacomp {

// TODO: Pick better seperator for utf8 and binary compability
const uint8_t CODE1_RULE_ID = 0x01;

using OutputSize = uint64_t;

static size_t bitsFor(size_t n) {
    // TODO: Maybe use nice bit ops?
    return size_t(ceil(log2(std::max(size_t(2), n))));
}

static size_t bytesFor(size_t bits) {
    // TODO: Maybe use nice bit ops?
    return (size_t) ceil(bits / 8.0);
}

/// Only write the first `bytes` bytes of `x`.
/// Defined such that for any integer type T` the output is the same
/// independent from endianness.
template<class T>
static void code1WriteInt(std::ostream& out, T x, size_t bytes = sizeof(T)) {
    // Test endianess
    const T big = 1;
    const char* start = (const char*) &big;

    if (*start == 1) {
        // LSB first, swap bytes
        char *p = (char*) &x;
        for (size_t i = 0; i < sizeof(T) / 2; i++) {
            uint8_t tmp = p[i];
            p[i] = p[sizeof(T) - i - 1];
            p[sizeof(T) - i - 1] = tmp;
        }
    }

    // MSB first, point at start of x bytes
    out.write(((const char*) &x) + sizeof(T) - bytes, bytes);
}

void Code1Coder::code(Rules rules, Input input, size_t threshold, std::ostream& out) {
    ////////////////////////////
    // original Java Codee constructor

    ssize_t longestSubstitution = 0;
    ssize_t largestRefAbs = 0;
    {
        size_t rawSymbolsLeft = input.size();

        for (Rule rule: rules) {
            longestSubstitution = std::max(longestSubstitution, ssize_t(rule.num));
            largestRefAbs = std::max(largestRefAbs, ssize_t(rule.source));
            rawSymbolsLeft -= rule.num;
        }
    }

    ////////////////////////////
    // original Java code1() method

    const uint8_t bytesPerRef = bytesFor(bitsFor(largestRefAbs));
    const uint8_t bytesPerLen = bytesFor(bitsFor(longestSubstitution));

    // Write length
    code1WriteInt<OutputSize>(out, input.size());

    // Write bytesPerLen and bytesPerRef packed into one byte
    out.put((bytesPerRef << 4) | bytesPerLen);

    size_t p = 0;
    for (Rule rule : rules) {
        // TODO: Make it copy a slice
        for (; p < rule.target; p++) {
            out.put(input[p]);
        }
        out.put(CODE1_RULE_ID);

        code1WriteInt<OutputSize>(out, rule.source, bytesPerRef);
        DLOG(INFO) << "source: " << rule.source << " bpr: " << int(bytesPerRef);
        code1WriteInt<OutputSize>(out, rule.num, bytesPerLen);

        p = rule.target + rule.num;
    }
    // TODO: Make it copy a slice
    for (; p < input.size(); p++) {
        out.put(input[p]);
    }

    /* Size of returned bytes
    const uint8_t bytesPerRule = 1 + bytesPerRef + bytesPerLen;
    const uint8_t bytesPerSymbol = 1;
    return sizeof(size_t) // text len
        + sizeof(uint8_t) // bytes per ref and bytes per len
        + (input.size() - totalSubstituted) * bytesPerSymbol
        + rules.size() + bytesPerRule;
    */
}

void Code1Coder::decode(std::istream& inp, std::ostream& out) {
    OutputSize length = read_bytes<OutputSize>(inp);
    DLOG(INFO) << "text length: " << length;

    uint8_t bytesPerRefLen = read_bytes<uint8_t>(inp);
    uint8_t bytesPerRef = bytesPerRefLen >> 4;
    uint8_t bytesPerLen = bytesPerRefLen & 0b00001111;
    DLOG(INFO) << "bytes per ref: " << int(bytesPerRef);
    DLOG(INFO) << "bytes per len: " << int(bytesPerLen);

    DecodeBuffer buffer(length);
    DLOG(INFO) << "building text:";

    // TODO: Replace get(c) stuff with a "read_byte_or_cause_truncation_error"
    // function
    char c;
    while (inp.get(c)) {
        if (c == CODE1_RULE_ID) {
            size_t source = read_bytes<size_t>(inp, bytesPerRef);
            CHECK(source < length);

            size_t num = read_bytes<size_t>(inp, bytesPerLen);
            CHECK(num > 0);

            buffer.push_rule(source, num);
        } else {
            buffer.push_decoded_byte(c);
        }
    }

    // Then, finish decoding and write it out
    buffer.write_to(out);
}

}
