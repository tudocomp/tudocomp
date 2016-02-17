#include <cmath>
#include <algorithm>

#include "glog/logging.h"

#include "code1.h"
#include "tudocomp.h"

namespace esacomp {

// TODO: Pick better seperator for utf8 and binary compability
const uint8_t CODE1_RULE_ID = 0x01;

using OutputSize = uint64_t;

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

void Code1Coder::code(Rules&& rules, Input& inp, Output& output) {
    auto i_guard = inp.as_view();
    auto o_guard = output.as_stream();
    auto input = *i_guard;
    auto& out = *o_guard;

    ////////////////////////////
    // original Java Code constructor

    ssize_t longestSubstitution = 0;
    ssize_t largestRefAbs = 0;
    {
        for (Rule rule: rules) {
            longestSubstitution = std::max(longestSubstitution, ssize_t(rule.num));
            largestRefAbs = std::max(largestRefAbs, ssize_t(rule.source));
        }
    }

    ////////////////////////////
    // original Java code1() method

    const uint8_t bytesPerRef = bytesFor(bitsFor(largestRefAbs));
    const uint8_t bytesPerLen = bytesFor(bitsFor(longestSubstitution));

    // define a predicate for filtering out rules
    // that end up taking more space encoded as raw.
    auto is_below_threshold = [&] (Rule& rule) -> bool {
        size_t threshold = 1 + bytesPerRef + bytesPerLen;
        return rule.num <= threshold;
    };

    // Write length
    code1WriteInt<OutputSize>(out, input.size());

    // Write bytesPerLen and bytesPerRef packed into one byte
    out.put((bytesPerRef << 4) | bytesPerLen);

    size_t p = 0;
    auto encode_raw_until = [&] (size_t up_to) {
        // TODO: Make it copy a slice
        for (; p <  up_to; p++) {
            out.put(input[p]);
        }
    };

    for (Rule rule : rules) {
        if (is_below_threshold(rule)) {
            continue;
        }

        encode_raw_until(rule.target);
        out.put(CODE1_RULE_ID);

        code1WriteInt<OutputSize>(out, rule.source, bytesPerRef);
        DLOG(INFO) << "source: " << rule.source << " bpr: " << int(bytesPerRef);
        code1WriteInt<OutputSize>(out, rule.num, bytesPerLen);

        p = rule.target + rule.num;
    }
    encode_raw_until(input.size());
}

void Code1Coder::decode(Input& input, Output& output) {
    auto i_guard = input.as_stream();
    auto o_guard = output.as_stream();
    auto& inp = *i_guard;
    auto& out = *o_guard;

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

size_t Code1Coder::min_encoded_rule_length(size_t input_size) {
    return 3; // rho + min_abs_ref + min_subs_len
}

}
