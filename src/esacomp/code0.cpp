#include <map>

#include "glog/logging.h"

#include "code0.h"
#include "rule.h"
#include "tudocomp.h"

namespace esacomp {

inline void write_and_escape_raw(boost::string_ref input, size_t abs_start, size_t abs_end, std::ostream& out) {
    size_t last_i = abs_start;
    for (size_t i = abs_start; i < abs_end; i++) {
        if (input[i] == '{') {
            out.write((const char*)&input[last_i], i - last_i);
            out.put('{');
            last_i = i;
        }
    }
    out.write((const char*)&input[last_i], abs_end - last_i);
}

void Code0Coder::code(Rules&& rules, Input& inp, Output& output) {
    // TODO: Somehow merge Rules and Input here so that input does not
    // get consumed twice
    auto i_guard = inp.as_view();
    auto o_guard = output.as_stream();
    auto input = *i_guard;
    auto& out = *o_guard;

    out << input.size() << ":";
    size_t start = 0;
    // TODO: Rule sort dependency
    for (Rule rule : rules) {
        write_and_escape_raw(input, start, rule.target, out);
        out << "{" << (rule.source + 1) << "," << rule.num << "}";
        start = rule.target + rule.num;
    }
    write_and_escape_raw(input, start, input.size(), out);
}

void Code0Coder::decode(Input& input, Output& output) {
    auto i_guard = input.as_stream();
    auto o_guard = output.as_stream();
    auto& inp = *i_guard;
    auto& out = *o_guard;

    char c;
    size_t length = parse_number_until_other(inp, c);
    CHECK_EQ(c, ':');

    DLOG(INFO) << "text len: " << length;

    DecodeBuffer buffer(length);

    // First, collect all rules and uncoded text from input:

    DLOG(INFO) << "building text:";

    while (inp.get(c)) {
        if (c == '{') {
            if (inp.get(c)) {
                if (c == '{') {
                    buffer.push_decoded_byte('{');
                } else {
                    inp.unget();
                    size_t source = parse_number_until_other(inp, c) - 1;
                    CHECK(source < length);
                    CHECK_EQ(c, ',');
                    size_t num = parse_number_until_other(inp, c);
                    CHECK_EQ(c, '}');
                    CHECK(num > 0);

                    buffer.push_rule(source, num);
                }
            }
        } else {
            buffer.push_decoded_byte(c);
        }
    }

    // Then, finish decoding and write it out
    buffer.write_to(out);
}

size_t Code0Coder::min_encoded_rule_length(size_t input_size) {
    return boost::string_ref("{0,0}").size();
}

}
