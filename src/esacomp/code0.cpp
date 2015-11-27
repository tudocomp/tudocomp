#include <map>

#include "glog/logging.h"

#include "code0.h"
#include "rule.h"
#include "tudocomp.h"

namespace esacomp {

inline void write_and_escape_raw(Input& input, size_t& p, std::ostream& out, size_t target) {
    size_t i = p;
    size_t last_i = p;
    for (; i < target - p; i++) {
        if (input[i] == '{') {
            out.write((const char*)&input[last_i], i - last_i);
            out.put('{');
            last_i = i;
        }
    }
    out.write((const char*)&input[last_i], target - last_i);
}

void Code0Coder::code(Rules rules, Input input, std::ostream& out) {
    out << input.size() << ":";
    size_t p = 0;
    // TODO: Rule sort dependecy
    for (Rule rule : rules) {
        write_and_escape_raw(input, p, out, rule.target);
        out << "{" << (rule.source + 1) << "," << rule.num << "}";
        p = rule.target + rule.num;
    }
    write_and_escape_raw(input, p, out, input.size());
}

void Code0Coder::decode(std::istream& inp, std::ostream& out) {
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
