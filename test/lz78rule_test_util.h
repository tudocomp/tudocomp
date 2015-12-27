#ifndef LZ78_RULE_TEST_UTIL_H_RULE_TEST_UTIL_H
#define LZ78_RULE_TEST_UTIL_H

namespace lz78rule_test {

using namespace lz_compressor;
using namespace esacomp;

template<class Comp, class Cod>
void lz78roundtrip(const std::string input_string) {
    Env env;

    Comp compressor { env };

    std::vector<uint8_t> input_vec { input_string.begin(), input_string.end() };
    Input input = Input::from_memory(input_vec);

    DLOG(INFO) << "LZ78 ROUNDTRIP TEXT: " << input_string;

    Entries entries = compressor.compress(input);

    DLOG(INFO) << "LZ78 ROUNDTRIP PRE ENTRIES";

    for (auto e : entries) {
        DLOG(INFO) << "LZ78 ROUNDTRIP ENTRY: " << e;
    }

    Cod coder { env };

    // Encode input with rules
    // TODO: rewrite ostream_to_string to output_to_string
    std::string coded_string = ostream_to_string([&] (std::ostream& out_) {
        Output out = Output::from_stream(out_);
        coder.code(std::move(entries), out);
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

}

#endif
